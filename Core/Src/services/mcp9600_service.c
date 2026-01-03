#include "services/mcp9600_service.h"

#include "drivers/mcp9600_driver.h"
#include "drivers/mcp9600_probe.h"

#include "stm32g4xx_hal.h"

static bool s_valid = false;
static uint32_t s_last_update_ms = 0;
static uint32_t s_next_sample_ms = 0;
static int32_t s_cold_junction_c_x100 = 0;

void mcp9600_service_init(void)
{
	s_valid = false;
	s_last_update_ms = 0;
	s_next_sample_ms = HAL_GetTick();
	s_cold_junction_c_x100 = 0;
}

void mcp9600_service_reset(void)
{
	mcp9600_service_init();
}

void mcp9600_service_tick(uint32_t now_ms)
{
	// Keep it simple: probe once per second.
	if ((int32_t)(now_ms - s_next_sample_ms) < 0) {
		return;
	}
	s_next_sample_ms = now_ms + 1000u;

	if (!mcp9600_probe_is_ready()) {
		s_valid = false;
		return;
	}

	int32_t temp_c_x100 = 0;
	if (!mcp9600_read_cold_junction_c_x100(&temp_c_x100)) {
		s_valid = false;
		return;
	}

	s_valid = true;
	s_last_update_ms = now_ms;
	s_cold_junction_c_x100 = temp_c_x100;
}

bool mcp9600_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}

bool mcp9600_service_get_cold_junction_c_x100(int32_t *out_c_x100)
{
	if ((out_c_x100 == NULL) || !s_valid) {
		return false;
	}
	*out_c_x100 = s_cold_junction_c_x100;
	return true;
}
