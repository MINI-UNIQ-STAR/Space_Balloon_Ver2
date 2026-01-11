#ifndef __MOCK_DEPENDENCIES_H
#define __MOCK_DEPENDENCIES_H

#include <stdint.h>
#include "sensors.h"

// Mock Control Functions
void MockHAL_SetTick(uint32_t tick);
void MockHAL_AdvanceTick(uint32_t delta);
void MockSensors_ClearStats(void);
int MockSensors_GetResetCount(void);
int MockSensors_GetLastResetSensor(void);
int MockGPIO_GetPMSSetState(void);

// Mock System Functions (used by FDIR)
uint32_t HAL_GetTick(void);
void Sensors_Reset(SensorID_t id);

#endif
