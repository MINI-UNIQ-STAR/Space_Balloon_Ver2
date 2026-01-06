#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Optional 1Hz debug log output over UART4 (TX only).
// This is intentionally gated so it never interferes with UART3 telemetry framing.
// Enable by setting UART4_DEBUG_LOG_ENABLE to 1 in your build (e.g. via compiler flags)
// and wiring/configuring UART4 TX pins as needed.
#ifndef UART4_DEBUG_LOG_ENABLE
#define UART4_DEBUG_LOG_ENABLE 0
#endif

void uart4_debug_log_init(void);
void uart4_debug_log_tick(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
