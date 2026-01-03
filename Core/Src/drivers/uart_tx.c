#include "drivers/uart_tx.h"

#include "main.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <string.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

enum {
	UART3_TX_RING_SIZE = 512u,
};

static uint8_t s_uart3_ring[UART3_TX_RING_SIZE];
static size_t s_uart3_head = 0u;
static size_t s_uart3_tail = 0u;

static bool s_uart3_tx_in_flight = false;
static volatile size_t s_uart3_tx_chunk_len = 0u;

static StaticSemaphore_t s_uart3_mutex_buf;
static SemaphoreHandle_t s_uart3_mutex;

static StaticSemaphore_t s_uart3_progress_sem_buf;
static SemaphoreHandle_t s_uart3_progress_sem;

static TickType_t to_ticks(uint32_t timeout_ms)
{
	if (timeout_ms == 0u) {
		return 0;
	}
	TickType_t t = pdMS_TO_TICKS(timeout_ms);
	return (t == 0) ? 1 : t;
}

static void uart3_tx_init_if_needed(void)
{
	if (s_uart3_mutex == NULL) {
		s_uart3_mutex = xSemaphoreCreateMutexStatic(&s_uart3_mutex_buf);
		configASSERT(s_uart3_mutex != NULL);
	}
	if (s_uart3_progress_sem == NULL) {
		s_uart3_progress_sem = xSemaphoreCreateBinaryStatic(&s_uart3_progress_sem_buf);
		configASSERT(s_uart3_progress_sem != NULL);
		// Start empty.
		(void)xSemaphoreTake(s_uart3_progress_sem, 0u);
	}
}

static size_t ring_used(void)
{
	if (s_uart3_head >= s_uart3_tail) {
		return s_uart3_head - s_uart3_tail;
	}
	return (UART3_TX_RING_SIZE - s_uart3_tail) + s_uart3_head;
}

static size_t ring_free(void)
{
	// Keep one byte empty to distinguish full vs empty.
	return (UART3_TX_RING_SIZE - 1u) - ring_used();
}

static void uart3_kick_tx_locked(void)
{
	if (s_uart3_tx_in_flight) {
		return;
	}
	const size_t used = ring_used();
	if (used == 0u) {
		return;
	}

	// Transmit the largest contiguous chunk from tail to end.
	size_t chunk = used;
	const size_t tail_to_end = UART3_TX_RING_SIZE - s_uart3_tail;
	if (chunk > tail_to_end) {
		chunk = tail_to_end;
	}

	s_uart3_tx_chunk_len = chunk;
	s_uart3_tx_in_flight = true;
	if (HAL_UART_Transmit_IT(&huart3, &s_uart3_ring[s_uart3_tail], (uint16_t)chunk) != HAL_OK) {
		// Failed to start TX; drop the in-flight flag and let callers retry later.
		s_uart3_tx_in_flight = false;
		s_uart3_tx_chunk_len = 0u;
	}
}

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

	if (len > (UART3_TX_RING_SIZE - 1u)) {
		return false;
	}

	uart3_tx_init_if_needed();

	const TickType_t deadline = (timeout_ms == 0u) ? 0u : (xTaskGetTickCount() + to_ticks(timeout_ms));
	while (true) {
		if (xSemaphoreTake(s_uart3_mutex, to_ticks(timeout_ms)) != pdTRUE) {
			return false;
		}

		const size_t free_bytes = ring_free();
		if (free_bytes >= len) {
			// Write into ring (may wrap): memcpy in up to 2 segments.
			size_t head = s_uart3_head;
			size_t first = UART3_TX_RING_SIZE - head;
			if (first > len) {
				first = len;
			}
			memcpy(&s_uart3_ring[head], data, first);
			const size_t rem = len - first;
			if (rem != 0u) {
				memcpy(&s_uart3_ring[0], &data[first], rem);
				head = rem;
			} else {
				head += first;
				if (head >= UART3_TX_RING_SIZE) {
					head = 0u;
				}
			}
			s_uart3_head = head;
			uart3_kick_tx_locked();
			(void)xSemaphoreGive(s_uart3_mutex);
			return true;
		}

		// Not enough space.
		(void)xSemaphoreGive(s_uart3_mutex);
		if (timeout_ms == 0u) {
			return false;
		}

		TickType_t now = xTaskGetTickCount();
		if ((int32_t)(deadline - now) <= 0) {
			return false;
		}
		// Wait for TX progress signaled from ISR.
		(void)xSemaphoreTake(s_uart3_progress_sem, deadline - now);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart != &huart3) {
		return;
	}

	BaseType_t higher_woken = pdFALSE;
	// Advance tail by completed chunk.
	const size_t chunk = s_uart3_tx_chunk_len;
	s_uart3_tail += chunk;
	if (s_uart3_tail >= UART3_TX_RING_SIZE) {
		s_uart3_tail -= UART3_TX_RING_SIZE;
	}
	// Mark idle before kicking next, so uart3_kick_tx_locked() will start it.
	s_uart3_tx_in_flight = false;
	s_uart3_tx_chunk_len = 0u;

	if (s_uart3_progress_sem != NULL) {
		(void)xSemaphoreGiveFromISR(s_uart3_progress_sem, &higher_woken);
	}

	// Start next chunk if any.
	if (s_uart3_mutex != NULL) {
		if (xSemaphoreTakeFromISR(s_uart3_mutex, &higher_woken) == pdTRUE) {
			uart3_kick_tx_locked();
			(void)xSemaphoreGiveFromISR(s_uart3_mutex, &higher_woken);
		}
	}

	portYIELD_FROM_ISR(higher_woken);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart != &huart3) {
		return;
	}

	BaseType_t higher_woken = pdFALSE;
	// Abort current transfer; allow future kicks.
	s_uart3_tx_in_flight = false;
	s_uart3_tx_chunk_len = 0u;
	if (s_uart3_progress_sem != NULL) {
		(void)xSemaphoreGiveFromISR(s_uart3_progress_sem, &higher_woken);
	}
	portYIELD_FROM_ISR(higher_woken);
}
