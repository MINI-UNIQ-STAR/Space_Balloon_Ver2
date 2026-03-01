#ifndef __ACTUATORS_H
#define __ACTUATORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// Heater Channels
// PA6: Battery Heater
// PC6: Board Heater

void Actuators_Init(void);
void Actuators_SetHeater_Battery(float duty_percent); // 0.0 to 100.0
void Actuators_SetHeater_Board(float duty_percent);   // 0.0 to 100.0

#ifdef __cplusplus
}
#endif

#endif /* __ACTUATORS_H */
