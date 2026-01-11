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
void FDIR_Update(void); /* Call at 1Hz or similar */
void FDIR_ReportSuccess(SensorID_t sensor_id); /* Called by sensors when valid data read */
void FDIR_ReportFailure(SensorID_t sensor_id, int32_t error_code);

// Status query functions
FdirState_t FDIR_GetSensorState(SensorID_t id);
uint32_t FDIR_GetRecoveryCount(SensorID_t id);
bool FDIR_IsSensorHealthy(SensorID_t id);
bool FDIR_IsSensorColdDisabled(SensorID_t id);

// Temperature input for thermal protection
void FDIR_UpdateTemperature(int16_t ext_temp_c_x100);

// ===== New APIs for FDIR.md compliance =====

// Range validation (returns true if valid, false if out of range)
bool FDIR_ValidateRange_Baro(uint32_t press_pa);
bool FDIR_ValidateRange_GPS_Alt(float alt_m);
bool FDIR_ValidateRange_Temp(int16_t temp_c_x100);

// Continuity check (returns true if continuous, false if jump detected)
bool FDIR_CheckContinuity_GPS_Alt(float new_alt_m);

// Fallback altitude source
float FDIR_GetBackupAltitude(void);
void FDIR_UpdateGPSAltitude(float gps_alt_m);
void FDIR_UpdateBaroAltitude(float baro_alt_m);

// System status flags (for telemetry)
uint16_t FDIR_GetStatusFlags(void);

// Status flag bit definitions
#define STATUS_SYS_OK         (1 << 0)
#define STATUS_GPS_WARN       (1 << 1)
#define STATUS_BARO_WARN      (1 << 2)
#define STATUS_IMU_WARN       (1 << 3)
#define STATUS_TEMP_WARN      (1 << 4)
#define STATUS_HEATER_ACTIVE  (1 << 5)
#define STATUS_LOW_BATTERY    (1 << 6)
#define STATUS_FDIR_RECOVERY  (1 << 7)
#define STATUS_ALT_JUMP       (1 << 8)
#define STATUS_RANGE_ERROR    (1 << 9)

#endif
