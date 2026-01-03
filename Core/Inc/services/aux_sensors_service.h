#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void aux_sensors_service_init(void);
void aux_sensors_service_tick(uint32_t now_ms);

bool aux_sensors_get_bat_temp_c_x100(int16_t *out);
bool aux_sensors_get_board_temp_c_x100(int16_t *out);
bool aux_sensors_get_bat_mv(uint16_t *out);

// Health monitoring helpers
bool aux_sensors_get_temp_last_update_ms(uint32_t *out_ms);
bool aux_sensors_get_bat_last_update_ms(uint32_t *out_ms);
void aux_sensors_service_reset_temp(void);

#ifdef __cplusplus
}
#endif
