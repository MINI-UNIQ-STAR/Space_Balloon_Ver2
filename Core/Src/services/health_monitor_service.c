#include "services/health_monitor_service.h"

#include "drivers/i2c_recovery.h"

#include "services/aux_sensors_service.h"
#include "services/alt_kf_service.h"
#include "services/co2_service.h"
#include "services/gps_service.h"
#include "services/gdk101_service.h"
#include "services/imu_service.h"
#include "services/mag_service.h"
#include "services/mcp9600_service.h"
#include "services/ms5611_service.h"
#include "services/ozone_service.h"
#include "services/pms3003_service.h"
#include "services/sht31_service.h"

#include "drivers/reset_lines.h"
#include "drivers/gps_int_capture.h"

#include "stm32g4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

enum {
	HEALTH_CHECK_PERIOD_MS = 100u,
	HEALTH_STALE_THRESHOLD_MS = 2000u,
	HEALTH_PMS_STALE_THRESHOLD_MS = 3000u,
	HEALTH_MAX_RECOVERY_ATTEMPTS = 5u,
	HEALTH_RESET_PULSE_LOW_MS = 5u,
	HEALTH_RESET_PULSE_HIGH_MS = 50u,
	HEALTH_RESET_PULSE_LOW2_MS = 5u,
	HEALTH_PMS_SET_LOW_MS = 1u,
};

typedef struct {
	bool ever_updated;
	uint8_t attempts;
	bool permfail;
} health_state_t;

static uint16_t s_flags = 0;
static uint16_t s_ext_flags = 0;
static uint32_t s_next_check_ms = 0;

static health_state_t s_temp_state;
static health_state_t s_gps_state;
static health_state_t s_pms_state;
static health_state_t s_sht31_state;
static health_state_t s_ms5611_state;
static health_state_t s_gdk101_state;
static health_state_t s_imu_state;
static health_state_t s_co2_state;
static health_state_t s_mcp9600_state;
static health_state_t s_mag_state;
static health_state_t s_ozone_state;

static bool is_stale(uint32_t now_ms, uint32_t last_ms, uint32_t threshold_ms)
{
	return (uint32_t)(now_ms - last_ms) >= threshold_ms;
}

static void bump_attempt_or_permfail(health_state_t *st)
{
	if ((st == NULL) || st->permfail) {
		return;
	}
	if (st->attempts < 0xFFu) {
		st->attempts++;
	}
	if (st->attempts >= HEALTH_MAX_RECOVERY_ATTEMPTS) {
		st->permfail = true;
	}
}

static bool recover_i2c3_bus(void)
{
	// I2C3 pins from .ioc:
	// SCL: PA8, SDA: PB5
	return i2c_recover_bus(&hi2c3, GPIOA, GPIO_PIN_8, GPIOB, GPIO_PIN_5);
}

static bool recover_i2c1_bus(void)
{
	// I2C1 pins from .ioc:
	// SCL: PA15, SDA: PA14
	return i2c_recover_bus(&hi2c1, GPIOA, GPIO_PIN_15, GPIOA, GPIO_PIN_14);
}

void health_monitor_service_init(void)
{
	s_flags = 0;
	s_ext_flags = 0;
	s_next_check_ms = 0;

	s_temp_state = (health_state_t){0};
	s_gps_state = (health_state_t){0};
	s_pms_state = (health_state_t){0};
	s_sht31_state = (health_state_t){0};
	s_ms5611_state = (health_state_t){0};
	s_gdk101_state = (health_state_t){0};
	s_imu_state = (health_state_t){0};
	s_co2_state = (health_state_t){0};
	s_mcp9600_state = (health_state_t){0};
	s_mag_state = (health_state_t){0};
	s_ozone_state = (health_state_t){0};
}

void health_monitor_service_tick(uint32_t now_ms)
{
	if ((int32_t)(now_ms - s_next_check_ms) < 0) {
		return;
	}
	s_next_check_ms = now_ms + HEALTH_CHECK_PERIOD_MS;

	uint16_t flags = 0;
	uint16_t ext_flags = 0;

	// TEMP (DS18B20 via aux_sensors_service)
	{
		uint32_t last_ms = 0;
		if (aux_sensors_get_temp_last_update_ms(&last_ms)) {
			s_temp_state.ever_updated = true;
		}
		if (s_temp_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_TEMP_INT;
		} else if (s_temp_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_TEMP_INT_STALE;
			aux_sensors_service_reset_temp();
			bump_attempt_or_permfail(&s_temp_state);
		}
	}

	// GPS (USART1)
	{
		uint32_t last_ms = 0;
		if (gps_service_get_last_update_ms(&last_ms)) {
			s_gps_state.ever_updated = true;
		}
		if (s_gps_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_GPS;
		} else if (s_gps_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_GPS_STALE;
			(void)HAL_UART_AbortReceive(&huart1);
			gps_service_reset();
			bump_attempt_or_permfail(&s_gps_state);
		}
	}

	// GPS INT (GPIO/EXTI): mark if an interrupt edge was observed recently.
	{
		uint32_t int_last_ms = 0;
		uint32_t int_count = 0;
		if (gps_int_capture_get_last(&int_last_ms, &int_count)) {
			(void)int_count;
			if (!is_stale(now_ms, int_last_ms, HEALTH_STALE_THRESHOLD_MS)) {
				ext_flags |= HEALTH_EXT_FLAG_GPS_INT_RECENT;
			}
		}
	}

	// PMS3003 (USART2)
	{
		uint32_t last_ms = 0;
		if (pms3003_service_get_last_update_ms(&last_ms)) {
			s_pms_state.ever_updated = true;
		}
		if (s_pms_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_PMS;
		} else if (s_pms_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_PMS_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_PMS_STALE;
			(void)HAL_UART_AbortReceive(&huart2);
			(void)reset_line_pulse_high_low_high(RESET_LINE_PMS_SET,
							0u,
							HEALTH_PMS_SET_LOW_MS,
							0u);
			pms3003_service_reset();
			bump_attempt_or_permfail(&s_pms_state);
		}
	}

	// SHT31 (I2C3)
	{
		uint32_t last_ms = 0;
		if (sht31_service_get_last_update_ms(&last_ms)) {
			s_sht31_state.ever_updated = true;
		}
		if (s_sht31_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_SHT31;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_SHT_RST, true);
		} else if (s_sht31_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_SHT31_STALE;
			(void)recover_i2c3_bus();
			(void)reset_line_pulse(RESET_LINE_SHT_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			sht31_service_reset();
			bump_attempt_or_permfail(&s_sht31_state);
		}
	}

	// MS5611 (I2C3)
	{
		uint32_t last_ms = 0;
		if (ms5611_service_get_last_update_ms(&last_ms)) {
			s_ms5611_state.ever_updated = true;
		}
		if (s_ms5611_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_MS5611;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_MS_RST, true);
		} else if (s_ms5611_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_MS5611_STALE;
			(void)recover_i2c3_bus();
			(void)reset_line_pulse(RESET_LINE_MS_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			ms5611_service_reset();
			bump_attempt_or_permfail(&s_ms5611_state);
		}
	}

	// MS5611 anomaly detection (Kalman innovation gating)
	if (alt_kf_service_is_baro_fault_active()) {
		flags |= HEALTH_FLAG_MS5611_ANOMALY;
	}

	// CO2 (CM1107N on I2C3)
	{
		uint32_t last_ms = 0;
		if (co2_service_get_last_update_ms(&last_ms)) {
			s_co2_state.ever_updated = true;
		}
		if (s_co2_state.permfail) {
			ext_flags |= HEALTH_EXT_FLAG_PERMFAIL_CO2;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_CO2_RST, true);
		} else if (s_co2_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			ext_flags |= HEALTH_EXT_FLAG_CO2_STALE;
			(void)recover_i2c3_bus();
			(void)reset_line_pulse(RESET_LINE_CO2_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			co2_service_reset();
			bump_attempt_or_permfail(&s_co2_state);
		}
	}

	// MCP9600 (I2C3)
	{
		uint32_t last_ms = 0;
		if (mcp9600_service_get_last_update_ms(&last_ms)) {
			s_mcp9600_state.ever_updated = true;
		}
		if (s_mcp9600_state.permfail) {
			ext_flags |= HEALTH_EXT_FLAG_PERMFAIL_MCP9600;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_MCP_RST, true);
		} else if (s_mcp9600_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			ext_flags |= HEALTH_EXT_FLAG_MCP9600_STALE;
			(void)recover_i2c3_bus();
			(void)reset_line_pulse(RESET_LINE_MCP_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			mcp9600_service_reset();
			bump_attempt_or_permfail(&s_mcp9600_state);
		}
	}

	// PMS3003 cold-temperature cutoff (based on MCP9600 internal/cold-junction temp)
	{
		int32_t temp_c_x100 = 0;
		if (!mcp9600_service_get_cold_junction_c_x100(&temp_c_x100)) {
			// Fail-safe: if MCP9600 temp is unavailable, keep PMS powered off.
			(void)reset_line_set(RESET_LINE_PMS_SET, false);
		} else if (temp_c_x100 < -2000) {
			(void)reset_line_set(RESET_LINE_PMS_SET, false);
		} else {
			(void)reset_line_set(RESET_LINE_PMS_SET, true);
		}
	}

	// GDK101 radiation (I2C1)
	{
		uint32_t last_ms = 0;
		if (gdk101_service_get_last_update_ms(&last_ms)) {
			s_gdk101_state.ever_updated = true;
		}
		if (s_gdk101_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_GDK101;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_SEN_RST, true);
		} else if (s_gdk101_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_GDK101_STALE;
			(void)recover_i2c1_bus();
			(void)reset_line_pulse(RESET_LINE_SEN_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			gdk101_service_reset();
			bump_attempt_or_permfail(&s_gdk101_state);
		}
	}

	// IMU (LSM6DSV16x on I2C1)
	{
		uint32_t last_ms = 0;
		if (imu_service_get_last_update_ms(&last_ms)) {
			s_imu_state.ever_updated = true;
		}
		if (s_imu_state.permfail) {
			flags |= HEALTH_FLAG_PERMFAIL_IMU;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_LSM_RST, true);
		} else if (s_imu_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			flags |= HEALTH_FLAG_IMU_STALE;
			(void)recover_i2c1_bus();
			(void)reset_line_pulse(RESET_LINE_LSM_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			imu_service_reset();
			bump_attempt_or_permfail(&s_imu_state);
		}
	}

	// MLX90393 Magnetometer (I2C1)
	{
		uint32_t last_ms = 0;
		if (mag_service_get_last_update_ms(&last_ms)) {
			s_mag_state.ever_updated = true;
		}
		if (s_mag_state.permfail) {
			ext_flags |= HEALTH_EXT_FLAG_PERMFAIL_MAG;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_MLX_RST, true);
		} else if (s_mag_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			ext_flags |= HEALTH_EXT_FLAG_MAG_STALE;
			(void)recover_i2c1_bus();
			(void)reset_line_pulse(RESET_LINE_MLX_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			mag_service_reset();
			bump_attempt_or_permfail(&s_mag_state);
		}
	}

	// Ozone Sensor SEN0321 (I2C3)
	{
		uint32_t last_ms = 0;
		if (ozone_service_get_last_update_ms(&last_ms)) {
			s_ozone_state.ever_updated = true;
		}
		if (s_ozone_state.permfail) {
			ext_flags |= HEALTH_EXT_FLAG_PERMFAIL_OZONE;
			// Power off sensor on permanent failure (P-MOS: HIGH = OFF)
			(void)reset_line_set(RESET_LINE_SEN_RST, true);
		} else if (s_ozone_state.ever_updated && is_stale(now_ms, last_ms, HEALTH_STALE_THRESHOLD_MS)) {
			ext_flags |= HEALTH_EXT_FLAG_OZONE_STALE;
			(void)recover_i2c3_bus();
			(void)reset_line_pulse(RESET_LINE_SEN_RST,
							HEALTH_RESET_PULSE_LOW_MS,
							HEALTH_RESET_PULSE_HIGH_MS,
							HEALTH_RESET_PULSE_LOW2_MS);
			ozone_service_reset();
			bump_attempt_or_permfail(&s_ozone_state);
		}
	}

	s_flags = flags;
	s_ext_flags = ext_flags;
}

uint16_t health_monitor_get_flags(void)
{
	return s_flags;
}

uint16_t health_monitor_get_ext_flags(void)
{
	return s_ext_flags;
}
