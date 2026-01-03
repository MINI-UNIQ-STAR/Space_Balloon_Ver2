#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void gdk101_service_init(void);
void gdk101_service_reset(void);

// Non-blocking sampling. Stores the most recent uSv/h reading.
void gdk101_service_tick(uint32_t now_ms);

bool gdk101_service_get_last_usvh_x100(uint16_t *out_usvh_x100);
bool gdk101_service_get_last_update_ms(uint32_t *out_ms);

#ifdef __cplusplus
}
#endif
