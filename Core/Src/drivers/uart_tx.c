#include "drivers/uart_tx.h"

#include "main.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

bool uart1_tx_write(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
	if ((data == NULL) || (len == 0U)) {
		return true;
	}

	if (len > 0xFFFFU) {
		return false;
	}

	HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len, timeout_ms);
	return (st == HAL_OK);
}

bool uart3_tx_write(const uint8_t *data, size_t len, uint32_t timeout_ms)
{
	if ((data == NULL) || (len == 0U)) {
		return true;
	}

	if (len > 0xFFFFU) {
		return false;
	}

	HAL_StatusTypeDef st = HAL_UART_Transmit(&huart3, (uint8_t *)data, (uint16_t)len, timeout_ms);
	return (st == HAL_OK);
}
