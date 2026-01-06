#include "drivers/uart_rx_poll.h"

#include "stm32g4xx_hal.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

bool uart1_rx_poll_read(uint8_t *buf, size_t max_len, size_t *out_len)
{
	if ((buf == NULL) || (out_len == NULL) || (max_len == 0u)) {
		return false;
	}

	*out_len = 0;

	for (size_t i = 0; i < max_len; i++) {
		uint8_t b = 0;
		const HAL_StatusTypeDef st = HAL_UART_Receive(&huart1, &b, 1, 0);
		if (st == HAL_OK) {
			buf[(*out_len)++] = b;
			continue;
		}
		if (st == HAL_TIMEOUT) {
			break;
		}

		// HAL_ERROR / HAL_BUSY: stop reading for now.
		break;
	}

	return true;
}

bool uart2_rx_poll_read(uint8_t *buf, size_t max_len, size_t *out_len)
{
	if ((buf == NULL) || (out_len == NULL) || (max_len == 0u)) {
		return false;
	}

	*out_len = 0;

	for (size_t i = 0; i < max_len; i++) {
		uint8_t b = 0;
		const HAL_StatusTypeDef st = HAL_UART_Receive(&huart2, &b, 1, 0);
		if (st == HAL_OK) {
			buf[(*out_len)++] = b;
			continue;
		}
		if (st == HAL_TIMEOUT) {
			break;
		}

		// HAL_ERROR / HAL_BUSY: stop reading for now.
		break;
	}

	return true;
}
