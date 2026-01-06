#include "services/sht31_service.h"

#include "drivers/sht31.h"
#include "drivers/sht31_codec.h"

#include "stm32g4xx_hal.h"

typedef enum {
	SHT31_STATE_IDLE = 0,
	SHT31_STATE_WAITING,
} sht31_state_t;

static sht31_state_t s_state;
static uint32_t s_next_sample_ms;
static uint32_t s_read_ready_ms;

static bool s_valid;
static int16_t s_temp_c_x100;
static uint16_t s_rh_x100;
static uint32_t s_last_update_ms;

#ifndef SHT31_SAMPLE_PERIOD_MS
// With the current task scheduling, a 100ms period is a practical default (>=10Hz class).
// If you need slower sampling or reduced bus usage, override at build time.
#define SHT31_SAMPLE_PERIOD_MS 100u
#endif

void sht31_service_init(void)
{
	s_state = SHT31_STATE_IDLE;
	s_next_sample_ms = HAL_GetTick();
	s_read_ready_ms = 0;

	s_valid = false;
	s_temp_c_x100 = 0;
	s_rh_x100 = 0;
	s_last_update_ms = 0;
}

void sht31_service_reset(void)
{
	sht31_service_init();
}

void sht31_service_tick(uint32_t now_ms)
{
	// Non-blocking state machine:
	// 1) send measure command
	// 2) after ~20ms, read 6 bytes
	//
	// NOTE: To achieve >=10Hz-class updates even when this tick is called at coarser periods,
	// we pipeline by: read -> (if due) start the next measurement in the same call.
	if (s_state == SHT31_STATE_IDLE) {
		if ((int32_t)(now_ms - s_next_sample_ms) < 0) {
			return;
		}

		if (sht31_start_measurement()) {
			s_state = SHT31_STATE_WAITING;
			s_read_ready_ms = now_ms + 20u;
			s_next_sample_ms = now_ms + SHT31_SAMPLE_PERIOD_MS;
		} else {
			// Backoff if bus/device not ready
			s_valid = false;
			s_next_sample_ms = now_ms + 1000u;
		}
		return;
	}

	if (s_state == SHT31_STATE_WAITING) {
		if ((int32_t)(now_ms - s_read_ready_ms) < 0) {
			return;
		}

		uint16_t raw_t = 0;
		uint16_t raw_rh = 0;
		if (sht31_read_raw(&raw_t, &raw_rh)) {
			s_temp_c_x100 = sht31_temp_c_x100_from_raw(raw_t);
			s_rh_x100 = sht31_rh_x100_from_raw(raw_rh);
			s_valid = true;
			s_last_update_ms = now_ms;
		} else {
			s_valid = false;
		}

		// Pipeline: if it's time for the next sample, immediately kick off the next conversion.
		if ((int32_t)(now_ms - s_next_sample_ms) >= 0) {
			if (sht31_start_measurement()) {
				s_state = SHT31_STATE_WAITING;
				s_read_ready_ms = now_ms + 20u;
				s_next_sample_ms = now_ms + SHT31_SAMPLE_PERIOD_MS;
				return;
			}
			// If we cannot start again, fall back to IDLE and backoff.
			s_valid = false;
			s_state = SHT31_STATE_IDLE;
			s_next_sample_ms = now_ms + 1000u;
			return;
		}

		s_state = SHT31_STATE_IDLE;
		return;
	}
}

bool sht31_service_get_last(int16_t *temp_c_x100, uint16_t *rh_x100)
{
	if ((temp_c_x100 == NULL) || (rh_x100 == NULL)) {
		return false;
	}
	if (!s_valid) {
		return false;
	}
	*temp_c_x100 = s_temp_c_x100;
	*rh_x100 = s_rh_x100;
	return true;
}

bool sht31_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}
