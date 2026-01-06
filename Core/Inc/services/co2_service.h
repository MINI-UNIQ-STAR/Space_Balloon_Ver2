#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void co2_service_init(void);
void co2_service_tick(uint32_t now_ms);

bool co2_service_get_ppm(uint16_t *out_ppm);
bool co2_service_get_last_update_ms(uint32_t *out_ms);
void co2_service_reset(void);

#ifdef __cplusplus
}
#endif
