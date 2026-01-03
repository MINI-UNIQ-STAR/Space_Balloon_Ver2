#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Simple Independent Watchdog wrapper.
//
// Usage (example):
//   iwdg_simple_init_ms(8000);
//   ... periodically ...
//   iwdg_simple_kick();
//
// Note: relies on LSI being available. This project already enables LSI in SystemClock_Config.

bool iwdg_simple_init_ms(uint32_t timeout_ms);
void iwdg_simple_kick(void);

#ifdef __cplusplus
}
#endif
