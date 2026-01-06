#include "services/telemetry_payload_store.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include <string.h>

static telemetry_payload_sensor_snapshot_t s_payload;

static StaticSemaphore_t s_mutex_buf;
static SemaphoreHandle_t s_mutex;

void telemetry_payload_store_init(void)
{
	memset(&s_payload, 0, sizeof(s_payload));
	// Static mutex: no heap usage.
	s_mutex = xSemaphoreCreateMutexStatic(&s_mutex_buf);
	configASSERT(s_mutex != NULL);
}

static TickType_t to_ticks(uint32_t timeout_ms)
{
	if (timeout_ms == 0u) {
		return 0;
	}
	// pdMS_TO_TICKS rounds down; add 1 tick so short timeouts still have a chance.
	TickType_t t = pdMS_TO_TICKS(timeout_ms);
	return (t == 0) ? 1 : t;
}

bool telemetry_payload_store_write_lock(uint32_t timeout_ms)
{
	if (s_mutex == NULL) {
		return false;
	}
	return (xSemaphoreTake(s_mutex, to_ticks(timeout_ms)) == pdTRUE);
}

void telemetry_payload_store_write_unlock(void)
{
	if (s_mutex == NULL) {
		return;
	}
	(void)xSemaphoreGive(s_mutex);
}

telemetry_payload_sensor_snapshot_t *telemetry_payload_store_write_ptr_unsafe(void)
{
	return &s_payload;
}

bool telemetry_payload_store_read_copy(telemetry_payload_sensor_snapshot_t *out, uint32_t timeout_ms)
{
	if ((out == NULL) || (s_mutex == NULL)) {
		return false;
	}
	if (xSemaphoreTake(s_mutex, to_ticks(timeout_ms)) != pdTRUE) {
		return false;
	}
	memcpy(out, &s_payload, sizeof(*out));
	(void)xSemaphoreGive(s_mutex);
	return true;
}
