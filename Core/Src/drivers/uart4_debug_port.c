#include "drivers/uart4_debug_port.h"

#include "stm32g4xx_hal.h"

#if UART4_DEBUG_PORT_ENABLE

static UART_HandleTypeDef s_huart4;
static bool s_inited = false;

bool uart4_debug_port_init(void)
{
	if (s_inited) {
		return true;
	}

	__HAL_RCC_UART4_CLK_ENABLE();

	s_huart4.Instance = UART4;
	s_huart4.Init.BaudRate = 115200;
	s_huart4.Init.WordLength = UART_WORDLENGTH_8B;
	s_huart4.Init.StopBits = UART_STOPBITS_1;
	s_huart4.Init.Parity = UART_PARITY_NONE;
	s_huart4.Init.Mode = UART_MODE_TX;
	s_huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	s_huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	s_huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	s_huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	// NOTE: For actual output, UART4 TX pin must be configured to the proper AF.
	// This module intentionally does not hardcode a pin mapping; set it via CubeMX
	// or add MSP/GPIO init once the board's UART4 pins are confirmed.
	if (HAL_UART_Init(&s_huart4) != HAL_OK) {
		return false;
	}

	s_inited = true;
	return true;
}

bool uart4_debug_port_write(const void *data, size_t len)
{
	if (!s_inited) {
		return false;
	}
	if (data == NULL || len == 0) {
		return true;
	}

	if (HAL_UART_Transmit(&s_huart4, (uint8_t *)data, (uint16_t)len, 20) != HAL_OK) {
		return false;
	}
	return true;
}

#else

bool uart4_debug_port_init(void)
{
	return false;
}

bool uart4_debug_port_write(const void *data, size_t len)
{
	(void)data;
	(void)len;
	return false;
}

#endif
