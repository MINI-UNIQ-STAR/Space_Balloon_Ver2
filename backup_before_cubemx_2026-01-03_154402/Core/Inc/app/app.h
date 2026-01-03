#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void app_init(void);
void app_tick(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
