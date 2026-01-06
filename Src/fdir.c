#include "fdir.h"
#include "main.h" // HAL_GetTick
#include <stdio.h>

static SystemHealth_t sys_health; // Note: Currently struct matches specific fields, but array based on ID is easier.
// Redefining SystemHealth to Array for generic loop
static SensorHealth_t sensors_health[SENSOR_ID_COUNT];

#define SENSOR_TIMEOUT_DEFAULT 3000 // 3 seconds

void FDIR_Init(void) {
    for(int i=0; i<SENSOR_ID_COUNT; i++) {
        sensors_health[i].last_valid_update_ms = HAL_GetTick();
        sensors_health[i].error_count = 0;
        sensors_health[i].recovery_count = 0;
        sensors_health[i].state = FDIR_STATE_HEALTHY;
        sensors_health[i].enabled = true;
    }
}

void FDIR_ReportSuccess(void *sensor_id_ptr) {
    // Cast transparently if passed as void*
    // Using simple int ID is better
    SensorID_t id = (SensorID_t)(uintptr_t)sensor_id_ptr;
    if (id >= SENSOR_ID_COUNT) return;
    
    sensors_health[id].last_valid_update_ms = HAL_GetTick();
    sensors_health[id].error_count = 0;
    if (sensors_health[id].state == FDIR_STATE_WARNING || sensors_health[id].state == FDIR_STATE_RECOVERY) {
        sensors_health[id].state = FDIR_STATE_HEALTHY;
    }
}

void FDIR_ReportFailure(void *sensor_id_ptr, int error_code) {
    SensorID_t id = (SensorID_t)(uintptr_t)sensor_id_ptr;
    if (id >= SENSOR_ID_COUNT) return;
    
    sensors_health[id].error_count++;
}

void FDIR_Update(void) {
    uint32_t now = HAL_GetTick();
    
    for(int i=0; i<SENSOR_ID_COUNT; i++) {
        if (!sensors_health[i].enabled) continue;
        if (sensors_health[i].state == FDIR_STATE_FAILURE_PERMANENT) continue;
        
        uint32_t diff = now - sensors_health[i].last_valid_update_ms;
        
        // Timeout Detection
        if (diff > SENSOR_TIMEOUT_DEFAULT) {
            sensors_health[i].state = FDIR_STATE_WARNING;
            
            // Trigger Recovery
            // printf("FDIR: Sensor %d Timed Out (%d ms). Recovery Attempt %d.\n", i, diff, sensors_health[i].recovery_count + 1);
            
            Sensors_Reset((SensorID_t)i);
            
            sensors_health[i].last_valid_update_ms = now; // Reset timer to give recovery time
            sensors_health[i].recovery_count++;
            sensors_health[i].state = FDIR_STATE_RECOVERY;
            
            if (sensors_health[i].recovery_count >= 5) {
                sensors_health[i].state = FDIR_STATE_FAILURE_PERMANENT;
                // printf("FDIR: Sensor %d PERMANENT FAILURE.\n", i);
            }
        }
    }
}
