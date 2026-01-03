#include "stm32g4xx_hal.h"

#include "drivers/gps_int_capture.h"
#include "drivers/pps_capture.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// Dispatch to interested modules.
	pps_capture_exti_callback(GPIO_Pin);
	gps_int_capture_exti_callback(GPIO_Pin);
}
