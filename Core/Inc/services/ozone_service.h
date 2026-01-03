#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ozone_service_init(void);
void ozone_service_tick(uint32_t now_ms);

bool ozone_service_get_ppb(int16_t *out_ppb);

#ifdef __cplusplus
}
#endif
