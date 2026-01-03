#include "drivers/gps_ctrl.h"

#include "app/board_pins.h"
#include "stm32g4xx_hal.h"

static void gps_gpio_write(GPIO_TypeDef *port, uint16_t pin, bool high)
{
	HAL_GPIO_WritePin(port, pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void gps_ctrl_init(void)
{
	// Conservative defaults:
	// - Deassert reset (nRST high)
	// - Assert wake high (treat high as enabled)
	gps_ctrl_set_nrst(true);
	gps_ctrl_set_wake(true);
}

void gps_ctrl_set_wake(bool level_high)
{
	gps_gpio_write(GPS_WAKE_GPIO_PORT, GPS_WAKE_GPIO_PIN, level_high);
}

void gps_ctrl_set_nrst(bool deassert_high)
{
	gps_gpio_write(GPS_nRST_GPIO_PORT, GPS_nRST_GPIO_PIN, deassert_high);
}

void gps_ctrl_pulse_reset(uint32_t low_ms)
{
	gps_ctrl_set_nrst(false);
	HAL_Delay(low_ms);
	gps_ctrl_set_nrst(true);
}
