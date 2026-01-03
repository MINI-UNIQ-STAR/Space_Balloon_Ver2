#include "services/co2_service.h"

#include "drivers/cm1107n.h"

#include "stm32g4xx_hal.h"

static bool s_valid = false;
static uint16_t s_ppm = 0;
static uint8_t s_status = 0;
static uint32_t s_last_update_ms = 0;
static uint32_t s_next_sample_ms = 0;

void co2_service_init(void)
{
	s_valid = false;
	s_ppm = 0;
	s_status = 0;
	s_last_update_ms = 0;
	s_next_sample_ms = HAL_GetTick();
}

void co2_service_reset(void)
{
	co2_service_init();
}

void co2_service_tick(uint32_t now_ms)
{
	// CM1107N supports 1s sampling; keep it simple.
	if ((int32_t)(now_ms - s_next_sample_ms) < 0) {
		return;
	}
	s_next_sample_ms = now_ms + 1000u;

	uint16_t ppm = 0;
	uint8_t status = 0;
	if (cm1107n_read_ppm(&ppm, &status)) {
		s_ppm = ppm;
		s_status = status;
		s_valid = true;
		s_last_update_ms = now_ms;
	} else {
		s_valid = false;
	}
}

bool co2_service_get_ppm(uint16_t *out_ppm)
{
	if ((out_ppm == NULL) || !s_valid) {
		return false;
	}
	*out_ppm = s_ppm;
	return true;
}

bool co2_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}
