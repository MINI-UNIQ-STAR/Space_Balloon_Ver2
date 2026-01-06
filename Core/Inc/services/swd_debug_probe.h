#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SWD-only debug probe: exposes key runtime values as global volatile symbols
// so they can be inspected via debugger (Watch/Live Expressions) without using UART.
//
// This must not touch UART3 framing and should be safe even in flight builds.
#ifndef SWD_DEBUG_PROBE_ENABLE
#define SWD_DEBUG_PROBE_ENABLE 0
#endif

void swd_debug_probe_init(void);
void swd_debug_probe_tick(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
