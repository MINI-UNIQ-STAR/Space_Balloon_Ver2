#include "drivers/i2c_bus_lock.h"

#include "FreeRTOS.h"
#include "semphr.h"

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;

static StaticSemaphore_t s_i2c1_mutex_buf;
static SemaphoreHandle_t s_i2c1_mutex;

static StaticSemaphore_t s_i2c3_mutex_buf;
static SemaphoreHandle_t s_i2c3_mutex;

static TickType_t to_ticks(uint32_t timeout_ms)
{
	if (timeout_ms == 0u) {
		return 0;
	}
	TickType_t t = pdMS_TO_TICKS(timeout_ms);
	return (t == 0) ? 1 : t;
}

void i2c_bus_lock_init(void)
{
	// Static mutexes: no heap usage.
	if (s_i2c1_mutex == NULL) {
		s_i2c1_mutex = xSemaphoreCreateMutexStatic(&s_i2c1_mutex_buf);
		configASSERT(s_i2c1_mutex != NULL);
	}
	if (s_i2c3_mutex == NULL) {
		s_i2c3_mutex = xSemaphoreCreateMutexStatic(&s_i2c3_mutex_buf);
		configASSERT(s_i2c3_mutex != NULL);
	}
}

static SemaphoreHandle_t pick_mutex(I2C_HandleTypeDef *hi2c)
{
	if (hi2c == &hi2c1) {
		return s_i2c1_mutex;
	}
	if (hi2c == &hi2c3) {
		return s_i2c3_mutex;
	}
	return NULL;
}

bool i2c_bus_take(I2C_HandleTypeDef *hi2c, uint32_t timeout_ms)
{
	SemaphoreHandle_t m = pick_mutex(hi2c);
	if (m == NULL) {
		return true; // unknown bus: don't block
	}
	return (xSemaphoreTake(m, to_ticks(timeout_ms)) == pdTRUE);
}

void i2c_bus_give(I2C_HandleTypeDef *hi2c)
{
	SemaphoreHandle_t m = pick_mutex(hi2c);
	if (m == NULL) {
		return;
	}
	(void)xSemaphoreGive(m);
}
