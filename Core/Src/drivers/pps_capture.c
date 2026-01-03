#include "drivers/pps_capture.h"

#include "stm32g4xx_hal.h"

static volatile uint32_t s_pps_seq = 0;
static volatile uint32_t s_last_pps_ms = 0;
static volatile uint32_t s_last_interval_ms = 0;
static volatile bool s_have_pps = false;
static volatile bool s_have_interval = false;

void pps_capture_init(void)
{
	// No hardware init here: EXTI pin/NVIC are configured in main.c USER CODE.
	// This function exists for symmetry and future expansion.
}

bool pps_capture_get_last(uint32_t *last_pps_ms, uint32_t *seq)
{
	if (!s_have_pps) {
		return false;
	}
	if (last_pps_ms) {
		*last_pps_ms = s_last_pps_ms;
	}
	if (seq) {
		*seq = s_pps_seq;
	}
	return true;
}

bool pps_capture_get_last_interval_ms(uint32_t *interval_ms)
{
	if (!s_have_interval) {
		return false;
	}
	if (interval_ms) {
		*interval_ms = s_last_interval_ms;
	}
	return true;
}

static void pps_capture_on_rising_edge_isr(uint32_t now_ms)
{
	const uint32_t prev_ms = s_last_pps_ms;
	s_last_pps_ms = now_ms;
	s_pps_seq++;
	s_have_pps = true;

	if (s_pps_seq >= 2u) {
		s_last_interval_ms = now_ms - prev_ms;
		s_have_interval = true;
	}
}

void pps_capture_exti_callback(uint16_t gpio_pin)
{
	if (gpio_pin != GPIO_PIN_4) {
		return;
	}
	pps_capture_on_rising_edge_isr(HAL_GetTick());
}
