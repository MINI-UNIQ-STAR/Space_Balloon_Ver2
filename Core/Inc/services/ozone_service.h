#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ozone_service_init(void);
void ozone_service_tick(uint32_t now_ms);
void ozone_service_reset(void);

bool ozone_service_get_ppb(int16_t *out_ppb);
bool ozone_service_get_last_update_ms(uint32_t *out_ms);

#ifdef __cplusplus
}
#endif
