#include "drivers/gps_int_capture.h"

#include "stm32g4xx_hal.h"

static volatile uint32_t s_count = 0;
static volatile uint32_t s_last_ms = 0;
static volatile bool s_have_any = false;

void gps_int_capture_init(void)
{
	// EXTI pin/NVIC configured in main.c USER CODE.
}

bool gps_int_capture_get_last(uint32_t *last_ms, uint32_t *count)
{
	if (!s_have_any) {
		return false;
	}
	if (last_ms) {
		*last_ms = s_last_ms;
	}
	if (count) {
		*count = s_count;
	}
	return true;
}

void gps_int_capture_exti_callback(uint16_t gpio_pin)
{
	if (gpio_pin != GPIO_PIN_12) {
		return;
	}
	s_count++;
	s_last_ms = HAL_GetTick();
	s_have_any = true;
}
