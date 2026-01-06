#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initializes GPS control GPIO policy (wake + reset lines).
void gps_ctrl_init(void);

// Sets WAKE control level.
void gps_ctrl_set_wake(bool level_high);

// Sets nRST control level (true=deassert/high, false=assert/low).
void gps_ctrl_set_nrst(bool deassert_high);

// Assert reset low then release.
void gps_ctrl_pulse_reset(uint32_t low_ms);

#ifdef __cplusplus
}
#endif
