#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void sht31_service_init(void);
void sht31_service_tick(uint32_t now_ms);

// Latest valid sample (fixed-point):
// - temp_c_x100: degC * 100
// - rh_x100: %RH * 100
bool sht31_service_get_last(int16_t *temp_c_x100, uint16_t *rh_x100);

// Health monitoring helpers
bool sht31_service_get_last_update_ms(uint32_t *out_ms);
void sht31_service_reset(void);

#ifdef __cplusplus
}
#endif
