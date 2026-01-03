#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "drivers/pms_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void pms3003_service_init(void);
void pms3003_service_tick(uint32_t now_ms);

bool pms3003_get_reading(pms_reading_t *out);

// Health monitoring helpers
bool pms3003_service_get_last_update_ms(uint32_t *out_ms);
void pms3003_service_reset(void);

#ifdef __cplusplus
}
#endif
