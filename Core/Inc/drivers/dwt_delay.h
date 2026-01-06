#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initializes DWT cycle counter for microsecond delays.
// Safe to call multiple times.
bool dwt_delay_init(void);

// Busy-wait delay in microseconds.
// Requires dwt_delay_init() has been called at least once.
void dwt_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif
