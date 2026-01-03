#include "services/ms5611_service.h"

#include "drivers/ms5611.h"
#include "drivers/ms5611_codec.h"

#include "stm32g4xx_hal.h"

typedef enum {
	MS5611_STATE_RESET = 0,
	MS5611_STATE_WAIT_RESET,
	MS5611_STATE_READ_PROM,
	MS5611_STATE_START_D1,
	MS5611_STATE_WAIT_D1,
	MS5611_STATE_READ_D1,
	MS5611_STATE_START_D2,
	MS5611_STATE_WAIT_D2,
	MS5611_STATE_READ_D2,
	MS5611_STATE_COMPUTE,
	MS5611_STATE_IDLE,
} ms5611_state_t;

static ms5611_state_t s_state;
static uint32_t s_deadline_ms;
static uint32_t s_next_cycle_ms;
static uint8_t s_prom_index;
static uint16_t s_prom[8];
static uint32_t s_d1;
static uint32_t s_d2;

static bool s_valid;
static int32_t s_temp_c_x100;
static uint32_t s_press_pa;
static int32_t s_alt_m;
static uint32_t s_last_update_ms;

#ifndef MS5611_P0_PA
#define MS5611_P0_PA 101325u
#endif

#ifndef MS5611_SERVICE_OSR
// Oversampling ratio (OSR) selection controls conversion time vs noise.
// Default to highest OSR for best quality; override at build time if you prefer faster updates:
//   -DMS5611_SERVICE_OSR=MS5611_OSR_2048
//   -DMS5611_SERVICE_OSR=MS5611_OSR_1024
#define MS5611_SERVICE_OSR MS5611_OSR_4096
#endif

static const ms5611_osr_t k_osr = (ms5611_osr_t)MS5611_SERVICE_OSR;

void ms5611_service_init(void)
{
	s_state = MS5611_STATE_RESET;
	s_deadline_ms = 0;
	s_next_cycle_ms = HAL_GetTick();
	s_prom_index = 0;
	for (int i = 0; i < 8; i++) {
		s_prom[i] = 0;
	}
	s_d1 = 0;
	s_d2 = 0;
	s_valid = false;
	s_temp_c_x100 = 0;
	s_press_pa = 0;
	s_alt_m = 0;
	s_last_update_ms = 0;
}

void ms5611_service_reset(void)
{
	ms5611_service_init();
}

void ms5611_service_tick(uint32_t now_ms)
{
	// Allow multiple state transitions per call when no waiting is required.
	// Bound the work to keep this tick from running away.
	for (int steps = 0; steps < 8; steps++) {
		switch (s_state) {
	case MS5611_STATE_RESET:
		if ((int32_t)(now_ms - s_next_cycle_ms) < 0) {
			return;
		}
		if (!ms5611_reset()) {
			s_valid = false;
			s_next_cycle_ms = now_ms + 1000u;
			return;
		}
		s_deadline_ms = now_ms + 3u; // datasheet: 2.8ms reload
		s_state = MS5611_STATE_WAIT_RESET;
		return;

	case MS5611_STATE_WAIT_RESET:
		if ((int32_t)(now_ms - s_deadline_ms) < 0) {
			return;
		}
		s_prom_index = 0;
		s_state = MS5611_STATE_READ_PROM;
		break;

	case MS5611_STATE_READ_PROM: {
		uint16_t w = 0;
		if (!ms5611_read_prom_word(s_prom_index, &w)) {
			s_valid = false;
			s_state = MS5611_STATE_RESET;
			s_next_cycle_ms = now_ms + 1000u;
			return;
		}
		s_prom[s_prom_index] = w;
		s_prom_index++;
		if (s_prom_index >= 8u) {
			if (!ms5611_prom_crc_ok(s_prom)) {
				s_valid = false;
				s_state = MS5611_STATE_RESET;
				s_next_cycle_ms = now_ms + 1000u;
				return;
			}
			s_state = MS5611_STATE_START_D1;
		}
		break;
	}

	case MS5611_STATE_START_D1:
		if (!ms5611_start_d1_conversion(k_osr)) {
			s_valid = false;
			s_state = MS5611_STATE_RESET;
			s_next_cycle_ms = now_ms + 1000u;
			return;
		}
		s_deadline_ms = now_ms + ms5611_conversion_time_ms(k_osr);
		s_state = MS5611_STATE_WAIT_D1;
		return;

	case MS5611_STATE_WAIT_D1:
		if ((int32_t)(now_ms - s_deadline_ms) < 0) {
			return;
		}
		s_state = MS5611_STATE_READ_D1;
		break;

	case MS5611_STATE_READ_D1:
		if (!ms5611_read_adc(&s_d1)) {
			s_valid = false;
			s_state = MS5611_STATE_RESET;
			s_next_cycle_ms = now_ms + 1000u;
			return;
		}
		s_state = MS5611_STATE_START_D2;
		break;

	case MS5611_STATE_START_D2:
		if (!ms5611_start_d2_conversion(k_osr)) {
			s_valid = false;
			s_state = MS5611_STATE_RESET;
			s_next_cycle_ms = now_ms + 1000u;
			return;
		}
		s_deadline_ms = now_ms + ms5611_conversion_time_ms(k_osr);
		s_state = MS5611_STATE_WAIT_D2;
		return;

	case MS5611_STATE_WAIT_D2:
		if ((int32_t)(now_ms - s_deadline_ms) < 0) {
			return;
		}
		s_state = MS5611_STATE_READ_D2;
		break;

	case MS5611_STATE_READ_D2:
		if (!ms5611_read_adc(&s_d2)) {
			s_valid = false;
			s_state = MS5611_STATE_RESET;
			s_next_cycle_ms = now_ms + 1000u;
			return;
		}
		s_state = MS5611_STATE_COMPUTE;
		break;

	case MS5611_STATE_COMPUTE: {
		int32_t t;
		uint32_t p;
		if (ms5611_compensate(s_prom, s_d1, s_d2, &t, &p)) {
			s_temp_c_x100 = t;
			s_press_pa = p;
			s_alt_m = ms5611_altitude_m_from_pressure(p, MS5611_P0_PA);
			s_valid = true;
			s_last_update_ms = now_ms;
		} else {
			s_valid = false;
		}
		// Start the next cycle as soon as the scheduler calls us again.
		s_state = MS5611_STATE_IDLE;
		s_next_cycle_ms = now_ms;
		break;
	}

	case MS5611_STATE_IDLE:
		if ((int32_t)(now_ms - s_next_cycle_ms) < 0) {
			return;
		}
		s_state = MS5611_STATE_START_D1;
		break;

	default:
		s_state = MS5611_STATE_RESET;
		return;
		}
	}
}

bool ms5611_service_get_last(int32_t *temp_c_x100, uint32_t *press_pa, int32_t *alt_m)
{
	if ((temp_c_x100 == NULL) || (press_pa == NULL) || (alt_m == NULL)) {
		return false;
	}
	if (!s_valid) {
		return false;
	}
	*temp_c_x100 = s_temp_c_x100;
	*press_pa = s_press_pa;
	*alt_m = s_alt_m;
	return true;
}

bool ms5611_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}
