#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initializes PPS capture support (safe to call multiple times).
void pps_capture_init(void);

// Returns true if at least one PPS pulse has been captured.
// seq increments on each captured pulse.
bool pps_capture_get_last(uint32_t *last_pps_ms, uint32_t *seq);

// Returns true if we have a measured interval (ms) between last two pulses.
bool pps_capture_get_last_interval_ms(uint32_t *interval_ms);

// Called from shared EXTI callback dispatcher.
void pps_capture_exti_callback(uint16_t gpio_pin);

#ifdef __cplusplus
}
#endif
