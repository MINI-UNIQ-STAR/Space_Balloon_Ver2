#include "drivers/iwdg_simple.h"

#include "stm32g4xx_hal.h"

// LSI is typically ~32 kHz, but can vary. We use the nominal value to size the timeout.
#ifndef IWDG_LSI_HZ_NOMINAL
#define IWDG_LSI_HZ_NOMINAL 32000u
#endif

static IWDG_HandleTypeDef s_hiwdg;
static bool s_enabled = false;

static bool configure_iwdg(uint32_t timeout_ms)
{
	// IWDG: counter clock = LSI / prescaler
	// reload = timeout_s * counter_clock - 1
	// reload is 12-bit (0..4095)
	static const struct {
		uint32_t prescaler;
		uint32_t div;
	} k_prescalers[] = {
		{IWDG_PRESCALER_4, 4u},
		{IWDG_PRESCALER_8, 8u},
		{IWDG_PRESCALER_16, 16u},
		{IWDG_PRESCALER_32, 32u},
		{IWDG_PRESCALER_64, 64u},
		{IWDG_PRESCALER_128, 128u},
		{IWDG_PRESCALER_256, 256u},
	};

	if (timeout_ms == 0u) {
		return false;
	}

	for (size_t i = 0; i < (sizeof(k_prescalers) / sizeof(k_prescalers[0])); i++) {
		uint32_t div = k_prescalers[i].div;
		uint32_t tick_hz = IWDG_LSI_HZ_NOMINAL / div;
		if (tick_hz == 0u) {
			continue;
		}

		// Use 64-bit math to avoid overflow: reload = timeout_ms * tick_hz / 1000 - 1
		uint64_t ticks = ((uint64_t)timeout_ms * (uint64_t)tick_hz) / 1000u;
		if (ticks == 0u) {
			continue;
		}
		if (ticks > 4096u) {
			continue;
		}
		uint32_t reload = (uint32_t)(ticks - 1u);

		s_hiwdg.Instance = IWDG;
		s_hiwdg.Init.Prescaler = k_prescalers[i].prescaler;
		s_hiwdg.Init.Reload = reload;
		s_hiwdg.Init.Window = IWDG_WINDOW_DISABLE;
		return (HAL_IWDG_Init(&s_hiwdg) == HAL_OK);
	}

	// Requested timeout too large for our nominal clock/prescaler set.
	return false;
}

bool iwdg_simple_init_ms(uint32_t timeout_ms)
{
	s_enabled = configure_iwdg(timeout_ms);
	return s_enabled;
}

void iwdg_simple_kick(void)
{
	if (!s_enabled) {
		return;
	}
	(void)HAL_IWDG_Refresh(&s_hiwdg);
}
