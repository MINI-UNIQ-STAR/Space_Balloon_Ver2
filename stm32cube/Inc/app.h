#ifndef __APP_H
#define __APP_H

#include "main.h" // Holds HAL includes and basic types

// Global Contexts (Exposed for Test/Debugging)
extern PID_HandleTypeDef hpid_bat;
extern PID_HandleTypeDef hpid_brd;
extern KF_Handle_t hkf;
extern telemetry_frame_t telem_frame;
extern float heater_battery_cmd;
extern float heater_board_cmd;

void App_Init(void);
void App_Loop(void);

#endif
