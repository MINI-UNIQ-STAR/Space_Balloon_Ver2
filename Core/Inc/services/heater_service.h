#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void heater_service_init(void);
void heater_service_tick(uint32_t now_ms);

// Battery Heater (Kapton)
void heater_bat_set_target(float temp_c);
float heater_bat_get_target(void);
float heater_bat_get_duty(void);

// Board Heater (Minibulb)
void heater_board_set_target(float temp_c);
float heater_board_get_target(void);
float heater_board_get_duty(void);

#ifdef __cplusplus
}
#endif
