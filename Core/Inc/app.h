#ifndef __APP_H
#define __APP_H

#include "main.h" // Holds HAL includes and basic types
#include "pid.h"
#include "kalman.h"
#include "telemetry.h"

// Global Contexts (Exposed for Test/Debugging)
extern PID_HandleTypeDef hpid_bat;
extern PID_HandleTypeDef hpid_brd;
extern KF_Handle_t hkf;
extern telemetry_frame_t telem_frame;
extern float heater_battery_cmd;
extern float heater_board_cmd;

// Heater Power Budget Protection
#define HEATER_BATT_MAX_DUTY  60.0f  // 60% duty cycle limit (power budget)
#define HEATER_BOARD_MAX_DUTY 100.0f // No limit for minibulb (TBD)

// Low Voltage Protection
extern uint8_t g_low_voltage_mode;
uint8_t App_IsLowVoltageMode(void);

void App_Init(void);
void App_Loop(void);

#endif
