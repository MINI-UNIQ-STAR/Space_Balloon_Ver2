#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ms5611_service_init(void);
void ms5611_service_tick(uint32_t now_ms);

// Latest valid sample:
// - temp_c_x100: degC * 100
// - press_pa: Pascal
// - alt_m: barometric altitude in meters (based on sea-level reference pressure)
bool ms5611_service_get_last(int32_t *temp_c_x100, uint32_t *press_pa, int32_t *alt_m);

// Health monitoring helpers
bool ms5611_service_get_last_update_ms(uint32_t *out_ms);
void ms5611_service_reset(void);

#ifdef __cplusplus
}
#endif
