#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "services/nmea_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

void gps_service_init(void);
void gps_service_tick(uint32_t now_ms);

// Copies the latest parsed GPS state.
// Returns true if at least one sentence has been parsed.
bool gps_service_get_state(nmea_gps_state_t *out);

// Health monitoring helpers
bool gps_service_get_last_update_ms(uint32_t *out_ms);
void gps_service_reset(void);

#ifdef __cplusplus
}
#endif
