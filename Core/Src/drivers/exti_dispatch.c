#include "stm32g4xx_hal.h"
#include "app/board_pins.h"

#include "drivers/gps_int_capture.h"
#include "drivers/pps_capture.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Dispatch to interested modules.
	if (GPIO_Pin == GPS_PPS_GPIO_PIN) {
		pps_capture_exti_callback(GPIO_Pin);
	} else if (GPIO_Pin == GPS_INT_GPIO_PIN) {
		gps_int_capture_exti_callback(GPIO_Pin);
	} else if (GPIO_Pin == LSM_INT_GPIO_PIN) {
		// TODO: Call LSM6DSV16x interrupt handler
	} else if (GPIO_Pin == MLX_INT_GPIO_PIN) {
		// TODO: Call MLX90393 interrupt handler
	}
}
