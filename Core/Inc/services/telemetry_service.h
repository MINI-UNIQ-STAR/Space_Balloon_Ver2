#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void telemetry_service_init(void);

// Call periodically from the main loop.
void telemetry_service_tick(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
