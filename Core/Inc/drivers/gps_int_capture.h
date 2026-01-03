#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void gps_int_capture_init(void);

// Returns true if at least one interrupt edge has been observed.
bool gps_int_capture_get_last(uint32_t *last_ms, uint32_t *count);

// Called from shared EXTI callback dispatcher.
void gps_int_capture_exti_callback(uint16_t gpio_pin);

#ifdef __cplusplus
}
#endif
