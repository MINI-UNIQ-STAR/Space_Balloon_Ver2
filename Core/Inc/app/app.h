#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_init(void);
void app_tick(uint32_t now_ms);

// Priority-based multi-tasking split.
// These functions keep the existing service APIs intact.
void app_realtime_tick(uint32_t now_ms);
void app_sensor_tick(uint32_t now_ms);
void app_system_tick(uint32_t now_ms);

// Creates FreeRTOS tasks and synchronization primitives.
// Call this from MX_FREERTOS_Init() (before the scheduler starts).
void app_rtos_create_tasks(void);

#ifdef __cplusplus
}
#endif
