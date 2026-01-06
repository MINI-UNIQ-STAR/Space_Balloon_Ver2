#ifndef __FDIR_H
#define __FDIR_H

#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"

typedef enum {
    FDIR_STATE_HEALTHY = 0,
    FDIR_STATE_WARNING,
    FDIR_STATE_RECOVERY,
    FDIR_STATE_FAILURE_PERMANENT
} FdirState_t;

typedef struct {
    uint32_t last_valid_update_ms;
    uint32_t error_count;
    uint32_t recovery_count;
    FdirState_t state;
    bool enabled;
} SensorHealth_t;

typedef struct {
    SensorHealth_t imu;
    SensorHealth_t baro;
    SensorHealth_t gps;
    SensorHealth_t co2;
    SensorHealth_t pms;
    SensorHealth_t sht;
    SensorHealth_t rad;
    SensorHealth_t ext_temp;
} SystemHealth_t;

void FDIR_Init(void);
void FDIR_Update(void); // Call at 1Hz or similar
void FDIR_ReportSuccess(void *sensor_id); // Called by sensors when valid data read
void FDIR_ReportFailure(void *sensor_id, int error_code);

#endif
