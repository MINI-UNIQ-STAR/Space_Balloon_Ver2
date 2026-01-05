#include "app/app.h"

#include "services/telemetry_service.h"
#include "services/telemetry_payload_store.h"
#include "services/health_monitor_service.h"
#include "services/gps_service.h"
#include "services/aux_sensors_service.h"
#include "services/air_quality_service.h"
#include "services/co2_service.h"
#include "services/ozone_service.h"
#include "services/heater_service.h"
#include "services/sht31_service.h"
#include "services/ms5611_service.h"
#include "services/gdk101_service.h"
#include "services/imu_service.h"
#include "services/alt_kf_service.h"
#include "services/mcp9600_service.h"
#include "services/mag_service.h"
#include "services/uart4_debug_log.h"
#include "services/swd_debug_probe.h"

#include "drivers/i2c_bus_lock.h"
#include "drivers/dwt_delay.h"

#include "i2c.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32g4xx_hal.h"

static void app_fix_i2c1_addressing_mode_7bit(void)
{
	// CubeMX generated init currently sets I2C1 to 10-bit addressing.
	// We re-init here to enforce 7-bit until the project is regenerated from .ioc.
	if (hi2c1.Init.AddressingMode == I2C_ADDRESSINGMODE_7BIT) {
		return;
	}

	(void)HAL_I2C_DeInit(&hi2c1);
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
		Error_Handler();
	}
	HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);
}

static void app_init_gdk101_power_gpio_pb2(void)
{
	// PB2 is used as GDK101 power control (P-MOS). HIGH = OFF, LOW = ON.
	GPIO_InitTypeDef init = {0};
	init.Pin = GPIO_PIN_2;
	init.Mode = GPIO_MODE_OUTPUT_PP;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &init);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
}

static uint16_t app_status_flags_from_health(uint16_t health_flags, uint16_t health_ext_flags)
{
	// reference/사양서.txt 7.1 Status Flags (Bitwise)
	// Bit 0: DS18B20 Error
	// Bit 1: I2C1 Error (Downside)
	// Bit 2: I2C3 Error (Upside)
	// Bit 3: GPS Error
	// Bit 4: PMS3003 Timeout
	// Bit 5-7: Permanent Failure Flags

	uint16_t out = 0;

	const bool ds18_err = ((health_flags & (HEALTH_FLAG_TEMP_INT_STALE | HEALTH_FLAG_PERMFAIL_TEMP_INT)) != 0u);
	const bool gps_err = ((health_flags & (HEALTH_FLAG_GPS_STALE | HEALTH_FLAG_PERMFAIL_GPS)) != 0u);
	const bool pms_err = ((health_flags & (HEALTH_FLAG_PMS_STALE | HEALTH_FLAG_PERMFAIL_PMS)) != 0u);

	const bool i2c1_err =
		((health_flags & (HEALTH_FLAG_GDK101_STALE | HEALTH_FLAG_IMU_STALE | HEALTH_FLAG_PERMFAIL_GDK101 | HEALTH_FLAG_PERMFAIL_IMU)) != 0u) ||
		((health_ext_flags & (HEALTH_EXT_FLAG_MAG_STALE | HEALTH_EXT_FLAG_PERMFAIL_MAG | HEALTH_EXT_FLAG_OZONE_STALE | HEALTH_EXT_FLAG_PERMFAIL_OZONE)) != 0u);

	const bool i2c3_err =
		((health_flags & (HEALTH_FLAG_SHT31_STALE | HEALTH_FLAG_MS5611_STALE | HEALTH_FLAG_PERMFAIL_SHT31 | HEALTH_FLAG_PERMFAIL_MS5611)) != 0u) ||
		((health_ext_flags & (HEALTH_EXT_FLAG_CO2_STALE | HEALTH_EXT_FLAG_PERMFAIL_CO2 | HEALTH_EXT_FLAG_MCP9600_STALE | HEALTH_EXT_FLAG_PERMFAIL_MCP9600)) != 0u);

	if (ds18_err) {
		out |= (1u << 0);
	}
	if (i2c1_err) {
		out |= (1u << 1);
	}
	if (i2c3_err) {
		out |= (1u << 2);
	}
	if (gps_err) {
		out |= (1u << 3);
	}
	if (pms_err) {
		out |= (1u << 4);
	}

	// Permanent failure summary bits (5..7)
	if ((health_flags & HEALTH_FLAG_PERMFAIL_TEMP_INT) != 0u) {
		out |= (1u << 5);
	}
	if (((health_flags & (HEALTH_FLAG_PERMFAIL_GDK101 | HEALTH_FLAG_PERMFAIL_IMU)) != 0u) ||
		((health_ext_flags & (HEALTH_EXT_FLAG_PERMFAIL_MAG | HEALTH_EXT_FLAG_PERMFAIL_OZONE)) != 0u)) {
		out |= (1u << 6);
	}
	if (((health_flags & (HEALTH_FLAG_PERMFAIL_SHT31 | HEALTH_FLAG_PERMFAIL_MS5611)) != 0u) ||
		((health_ext_flags & (HEALTH_EXT_FLAG_PERMFAIL_CO2 | HEALTH_EXT_FLAG_PERMFAIL_MCP9600)) != 0u)) {
		out |= (1u << 7);
	}

	return out;
}

void app_init(void)
{
	i2c_bus_lock_init();
	app_init_gdk101_power_gpio_pb2();
	app_fix_i2c1_addressing_mode_7bit();
	(void)dwt_delay_init();
	telemetry_payload_store_init();

	gps_service_init();
	aux_sensors_service_init();
	air_quality_service_init();
	co2_service_init();
	ozone_service_init();
	heater_service_init();
	gdk101_service_init();
	sht31_service_init();
	ms5611_service_init();
	imu_service_init();
	mag_service_init();
	mcp9600_service_init();
	alt_kf_service_init();
	uart4_debug_log_init();
	swd_debug_probe_init();

	health_monitor_service_init();
	telemetry_service_init();
}

void app_tick(uint32_t now_ms)
{
	// Backward-compatible super-loop entry.
	// In the refactored RTOS build, these are driven by separate tasks.
	app_realtime_tick(now_ms);
	app_sensor_tick(now_ms);
	app_system_tick(now_ms);
}

void app_realtime_tick(uint32_t now_ms)
{
	// Measure realtime loop execution time (for offline analysis without hardware).
	// Uses DWT cycle counter; previously reported in payload.reserved1.
	const uint32_t cycles_per_us = (uint32_t)(SystemCoreClock / 1000000u);
	const uint32_t start_cycles = DWT->CYCCNT;

	// Latency-sensitive paths: keep these in the highest-priority 50Hz task.
	gps_service_tick(now_ms);
	imu_service_tick(now_ms);
	swd_debug_probe_tick(now_ms);
	uart4_debug_log_tick(now_ms);

	(void)cycles_per_us;
	(void)start_cycles;

	// Update the shared telemetry payload with realtime fields.
	// IMPORTANT: never block this task for long.
	if (telemetry_payload_store_write_lock(0u)) {
		telemetry_payload_sensor_snapshot_t *p = telemetry_payload_store_write_ptr_unsafe();
		p->uptime_ms = now_ms;
		{
			const uint16_t health = health_monitor_get_flags();
			const uint16_t ext = health_monitor_get_ext_flags();
			p->status_flags = app_status_flags_from_health(health, ext);
			// Protocol-stable detail fields:
			// - reserved1: ext health flags (16-bit)
			// - reserved2/3: internal health flags (16-bit)
			p->reserved1 = (int16_t)ext;
			p->reserved2 = (uint8_t)(health & 0xFFu);
			p->reserved3 = (uint8_t)((health >> 8) & 0xFFu);
		}

		int32_t accel[3];
		if (imu_service_get_accel_mps2_x1000(accel)) {
			for (int i = 0; i < 3; i++) {
				p->accel_mps2_x1000[i] = accel[i];
			}
		}
		int32_t gyro[3];
		if (imu_service_get_gyro_rads_x1000(gyro)) {
			for (int i = 0; i < 3; i++) {
				p->gyro_rads_x1000[i] = gyro[i];
			}
		}

		float mag[3];
		if (mag_service_get_data(&mag[0], &mag[1], &mag[2])) {
			p->mag_uT[0] = mag[0];
			p->mag_uT[1] = mag[1];
			p->mag_uT[2] = mag[2];
		}

		nmea_gps_state_t gps;
		if (gps_service_get_state(&gps)) {
			p->gps_fix = gps.has_fix ? 1u : 0u;
			p->gps_lat_deg_e7 = gps.lat_deg_e7;
			p->gps_lon_deg_e7 = gps.lon_deg_e7;
			p->gps_alt_m = (float)gps.alt_mm / 1000.0f;
			p->gps_sats_used = gps.sats_used;
			p->gps_sats_in_view_total = gps.sats_in_view_total;
			p->gps_sats_in_view_gps = gps.sats_in_view_gps;
			p->gps_sats_in_view_glonass = gps.sats_in_view_glonass;
			p->gps_sats_in_view_galileo = gps.sats_in_view_galileo;
			p->gps_sats_in_view_beidou = gps.sats_in_view_beidou;
		} else {
			p->gps_fix = 0u;
			p->gps_lat_deg_e7 = 0;
			p->gps_lon_deg_e7 = 0;
			p->gps_alt_m = 0.0f;
			p->gps_sats_used = 0u;
			p->gps_sats_in_view_total = 0u;
			p->gps_sats_in_view_gps = 0u;
			p->gps_sats_in_view_glonass = 0u;
			p->gps_sats_in_view_galileo = 0u;
			p->gps_sats_in_view_beidou = 0u;
		}

		float roll, pitch;
		if (imu_service_get_attitude(&roll, &pitch)) {
			p->kf_roll_deg = roll;
			p->kf_pitch_deg = pitch;
		}

		telemetry_payload_store_write_unlock();
	}

	// Telemetry send at 50Hz. Must not be blocked by slow sensors.
	telemetry_service_tick(now_ms);
}

void app_sensor_tick(uint32_t now_ms)
{
	// Sensors / slower services (some may block on I2C/UART timeouts)
	// Distribute work to avoid bunching multiple I2C transactions.
	// Base tick: 25ms
	// - every 25ms: baro state machine progression (enables ~20Hz class MS5611 updates)
	// - every 200ms: MCP9600
	// - every 500ms: CO2/Ozone/Air quality + radiation
	static uint32_t s_sensor_tick_25ms = 0;
	const bool do_payload_50ms = ((s_sensor_tick_25ms % 2u) == 0u);
	const bool do_200ms = ((s_sensor_tick_25ms % 8u) == 0u);
	const bool do_500ms = ((s_sensor_tick_25ms % 20u) == 0u);
	if (s_sensor_tick_25ms == 0xFFFFFFFFu) {
		s_sensor_tick_25ms = 0u;
	}
	// Advance for next call.
	s_sensor_tick_25ms++;

	aux_sensors_service_tick(now_ms);
	ms5611_service_tick(now_ms);
	sht31_service_tick(now_ms);
	alt_kf_service_tick(now_ms);

	if (do_200ms) {
		mcp9600_service_tick(now_ms);
	}
	if (do_500ms) {
		// I2C1 sensors (avoid colliding with 50Hz IMU reads on I2C1)
		mag_service_tick(now_ms);
		gdk101_service_tick(now_ms);
		co2_service_tick(now_ms);
		ozone_service_tick(now_ms);
		air_quality_service_tick(now_ms);
	}

	// Update shared payload with non-realtime sensor fields.
	// Throttle this to reduce mutex contention with the 50Hz task.
	if (do_payload_50ms && telemetry_payload_store_write_lock(5u)) {
		telemetry_payload_sensor_snapshot_t *p = telemetry_payload_store_write_ptr_unsafe();

		int16_t t_int;
		if (aux_sensors_get_bat_temp_c_x100(&t_int)) {
			p->bat_temp_c_x100 = t_int;
		}
		if (aux_sensors_get_board_temp_c_x100(&t_int)) {
			p->board_temp_c_x100 = t_int;
		}
		uint16_t bat_mv;
		if (aux_sensors_get_bat_mv(&bat_mv)) {
			p->bat_mv = bat_mv;
		}

		int16_t t_ext;
		uint16_t rh;
		if (sht31_service_get_last(&t_ext, &rh)) {
			p->sht31_temp_c_x100 = t_ext;
			p->sht31_rh_x100 = rh;
		}

		int32_t t_baro;
		uint32_t p_baro;
		int32_t alt_m;
		if (ms5611_service_get_last(&t_baro, &p_baro, &alt_m)) {
			p->ms5611_temp_c_x100 = (int16_t)t_baro;
			p->ms5611_press_pa = p_baro;
			p->press_alt_m = (float)alt_m;
		}

		float kf_alt;
		if (alt_kf_service_get_alt_m(&kf_alt)) {
			p->kf_alt_m = kf_alt;
		}

		if (do_200ms) {
			int32_t t_mcp;
			if (mcp9600_service_get_cold_junction_c_x100(&t_mcp)) {
				p->indoor_2nd_temp_c_x100 = (int16_t)t_mcp;
			}
			int32_t t_mcp_hot;
			if (mcp9600_service_get_hot_junction_c_x100(&t_mcp_hot)) {
				p->external_temp_c_x100 = (int16_t)t_mcp_hot;
			}
		}

		if (do_500ms) {
			uint16_t co2_ppm;
			if (co2_service_get_ppm(&co2_ppm)) {
				p->co2_ppm = co2_ppm;
			}

			uint16_t rad_x100;
			if (gdk101_service_get_last_usvh_x100(&rad_x100)) {
				p->gdk101_usvh_x100 = rad_x100;
			}

			pms_reading_t pm;
			if (air_quality_get_pm(&pm) && pm.valid) {
				p->pm1_ugm3 = pm.pm1_ugm3;
				p->pm25_ugm3 = pm.pm25_ugm3;
				p->pm10_ugm3 = pm.pm10_ugm3;
			}
		}

		telemetry_payload_store_write_unlock();
	}
}

void app_system_tick(uint32_t now_ms)
{
	// Low-priority housekeeping.
	heater_service_tick(now_ms);
	health_monitor_service_tick(now_ms);

	if (telemetry_payload_store_write_lock(10u)) {
		telemetry_payload_sensor_snapshot_t *p = telemetry_payload_store_write_ptr_unsafe();
		p->heater_bat_duty_percent = (uint8_t)(heater_bat_get_duty() * 100.0f);
		p->heater_board_duty_percent = (uint8_t)(heater_board_get_duty() * 100.0f);
		telemetry_payload_store_write_unlock();
	}
}

// --- FreeRTOS Task entry points ---

enum {
	APP_STACK_REALTIME_WORDS = 512u, // 2 KB
	APP_STACK_SENSOR_WORDS = 768u,   // 3 KB
	APP_STACK_SYSTEM_WORDS = 384u,   // 1.5 KB
};

enum {
	APP_PRIO_REALTIME = (tskIDLE_PRIORITY + 5),
	APP_PRIO_SENSOR = (tskIDLE_PRIORITY + 3),
	APP_PRIO_SYSTEM = (tskIDLE_PRIORITY + 1),
};

static StaticTask_t s_rt_tcb;
static StackType_t s_rt_stack[APP_STACK_REALTIME_WORDS];
static TaskHandle_t s_rt_handle;

static StaticTask_t s_sensor_tcb;
static StackType_t s_sensor_stack[APP_STACK_SENSOR_WORDS];
static TaskHandle_t s_sensor_handle;

static StaticTask_t s_system_tcb;
static StackType_t s_system_stack[APP_STACK_SYSTEM_WORDS];
static TaskHandle_t s_system_handle;

static void StartRealTimeTask(void *argument)
{
	(void)argument;
	TickType_t last_wake = xTaskGetTickCount();
	const TickType_t period = pdMS_TO_TICKS(20);
	for (;;) {
		const uint32_t now_ms = HAL_GetTick();
		app_realtime_tick(now_ms);
		vTaskDelayUntil(&last_wake, period);
	}
}

static void StartSensorTask(void *argument)
{
	(void)argument;
	TickType_t last_wake = xTaskGetTickCount();
	const TickType_t period = pdMS_TO_TICKS(25);
	for (;;) {
		const uint32_t now_ms = HAL_GetTick();
		app_sensor_tick(now_ms);
		vTaskDelayUntil(&last_wake, period);
	}
}

static void StartSystemTask(void *argument)
{
	(void)argument;
	TickType_t last_wake = xTaskGetTickCount();
	const TickType_t period = pdMS_TO_TICKS(1000);
	for (;;) {
		const uint32_t now_ms = HAL_GetTick();
		app_system_tick(now_ms);
		vTaskDelayUntil(&last_wake, period);
	}
}

void app_rtos_create_tasks(void)
{
	// NOTE: Static tasks avoid heap usage (important with 32KB RAM).
	s_rt_handle = xTaskCreateStatic(
			StartRealTimeTask,
			"RealTime",
			APP_STACK_REALTIME_WORDS,
			NULL,
			APP_PRIO_REALTIME,
			s_rt_stack,
			&s_rt_tcb);
	configASSERT(s_rt_handle != NULL);

	s_sensor_handle = xTaskCreateStatic(
			StartSensorTask,
			"Sensor",
			APP_STACK_SENSOR_WORDS,
			NULL,
			APP_PRIO_SENSOR,
			s_sensor_stack,
			&s_sensor_tcb);
	configASSERT(s_sensor_handle != NULL);

	s_system_handle = xTaskCreateStatic(
			StartSystemTask,
			"System",
			APP_STACK_SYSTEM_WORDS,
			NULL,
			APP_PRIO_SYSTEM,
			s_system_stack,
			&s_system_tcb);
	configASSERT(s_system_handle != NULL);
}
