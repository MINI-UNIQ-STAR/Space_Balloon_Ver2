#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "drivers/pms_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void air_quality_service_init(void);
void air_quality_service_tick(uint32_t now_ms);

bool air_quality_get_pm(pms_reading_t *out);

#ifdef __cplusplus
}
#endif
