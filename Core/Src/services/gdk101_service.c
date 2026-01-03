#include "services/gdk101_service.h"

#include "drivers/gdk101.h"

#include "stm32g4xx_hal.h"

static uint32_t s_next_sample_ms;
static bool s_valid;
static uint16_t s_usvh_x100;
static uint32_t s_last_update_ms;

void gdk101_service_init(void)
{
	s_next_sample_ms = HAL_GetTick();
	s_valid = false;
	s_usvh_x100 = 0;
	s_last_update_ms = 0;
}

void gdk101_service_reset(void)
{
	gdk101_service_init();
}

void gdk101_service_tick(uint32_t now_ms)
{
	// Sample at 1 Hz. Sensor itself updates once per minute, but polling faster is fine.
	if ((int32_t)(now_ms - s_next_sample_ms) < 0) {
		return;
	}
	uint16_t x100 = 0;
	if (gdk101_read_usvh_x100(&x100, false)) {
		s_usvh_x100 = x100;
		s_valid = true;
		s_last_update_ms = now_ms;
	} else {
		s_valid = false;
	}

	s_next_sample_ms = now_ms + 1000u;
}

bool gdk101_service_get_last_usvh_x100(uint16_t *out_usvh_x100)
{
	if (out_usvh_x100 == NULL) {
		return false;
	}
	if (!s_valid) {
		return false;
	}
	*out_usvh_x100 = s_usvh_x100;
	return true;
}

bool gdk101_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}
