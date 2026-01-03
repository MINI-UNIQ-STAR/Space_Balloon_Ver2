#include "drivers/dwt_delay.h"

#include "stm32g4xx_hal.h"

bool dwt_delay_init(void)
{
	// Enable TRC
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	// Reset and enable cycle counter
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	return true;
}

void dwt_delay_us(uint32_t us)
{
	const uint32_t cycles_per_us = (uint32_t)(SystemCoreClock / 1000000u);
	const uint32_t start = DWT->CYCCNT;
	const uint32_t target = us * cycles_per_us;
	while ((DWT->CYCCNT - start) < target) {
		// busy wait
	}
}
