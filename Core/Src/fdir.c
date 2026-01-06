#include "fdir.h"
#include "main.h"
#include <stdio.h>

static SystemHealth_t sys_health;
static SensorHealth_t sensors_health[SENSOR_ID_COUNT];

// Sensor-specific timeout configuration (in milliseconds)
static const uint32_t sensor_timeout_ms[SENSOR_ID_COUNT] = {
    [SENSOR_ID_IMU]      = 100,    // 480Hz → 100ms timeout
    [SENSOR_ID_MAG]      = 500,    // 50Hz → 500ms timeout
    [SENSOR_ID_BARO]     = 1000,   // 5Hz → 1s timeout
    [SENSOR_ID_GPS]      = 5000,   // 1Hz + cold start → 5s timeout
    [SENSOR_ID_PMS]      = 5000,   // Warm-up required → 5s timeout
    [SENSOR_ID_CO2]      = 5000,   // Warm-up required → 5s timeout
    [SENSOR_ID_SHT]      = 3000,   // 1Hz → 3s timeout
    [SENSOR_ID_RAD]      = 3000,   // 1Hz → 3s timeout
    [SENSOR_ID_EXT_TEMP] = 3000,   // 1Hz → 3s timeout
};

// Max recovery attempts per sensor (reduced for faster fail)
static const uint8_t sensor_max_recovery[SENSOR_ID_COUNT] = {
    [SENSOR_ID_IMU]      = 2,      // Critical, fast fail
    [SENSOR_ID_MAG]      = 2,
    [SENSOR_ID_BARO]     = 2,      // Critical for altitude
    [SENSOR_ID_GPS]      = 3,      // GPS can take time
    [SENSOR_ID_PMS]      = 1,      // Non-critical, fast fail
    [SENSOR_ID_CO2]      = 1,      // Non-critical
    [SENSOR_ID_SHT]      = 2,
    [SENSOR_ID_RAD]      = 2,
    [SENSOR_ID_EXT_TEMP] = 2,
};

// Operating temperature limits (in °C x 100)
// Format: {min_temp, max_temp}
static const int16_t sensor_temp_limits[SENSOR_ID_COUNT][2] = {
    [SENSOR_ID_IMU]      = {-4000, 8500},   // LSM6DSV16X: -40°C ~ +85°C
    [SENSOR_ID_MAG]      = {-4000, 8500},   // MLX90393: -40°C ~ +85°C
    [SENSOR_ID_BARO]     = {-4000, 8500},   // MS5611: -40°C ~ +85°C
    [SENSOR_ID_GPS]      = {-4000, 8500},   // XA1110: -40°C ~ +85°C
    [SENSOR_ID_PMS]      = {-1000, 6000},   // PMS3003: -10°C ~ +60°C ★
    [SENSOR_ID_CO2]      = {-500,  5000},   // CM1107N: -5°C ~ +50°C ★
    [SENSOR_ID_SHT]      = {-4000, 12500},  // SHT31: -40°C ~ +125°C
    [SENSOR_ID_RAD]      = {-2000, 6000},   // GDK101: -20°C ~ +60°C ★
    [SENSOR_ID_EXT_TEMP] = {-4000, 15000},  // MCP9600: -40°C ~ +150°C
};

// Temperature hysteresis (5°C = 500 in x100 scale)
#define TEMP_HYSTERESIS_X100  500

// Track cold-disabled state separately
static bool sensor_cold_disabled[SENSOR_ID_COUNT] = {false};

// Current external temperature (updated by FDIR_UpdateTemperature)
static int16_t current_ext_temp_x100 = 2500; // Default 25°C

void FDIR_Init(void) {
    for(int i = 0; i < SENSOR_ID_COUNT; i++) {
        sensors_health[i].last_valid_update_ms = HAL_GetTick();
        sensors_health[i].error_count = 0;
        sensors_health[i].recovery_count = 0;
        sensors_health[i].state = FDIR_STATE_HEALTHY;
        sensors_health[i].enabled = true;
        sensor_cold_disabled[i] = false;
    }
}

void FDIR_UpdateTemperature(int16_t ext_temp_c_x100) {
    current_ext_temp_x100 = ext_temp_c_x100;
}

void FDIR_ReportSuccess(void *sensor_id_ptr) {
    SensorID_t id = (SensorID_t)(uintptr_t)sensor_id_ptr;
    if (id >= SENSOR_ID_COUNT) return;
    
    sensors_health[id].last_valid_update_ms = HAL_GetTick();
    sensors_health[id].error_count = 0;
    if (sensors_health[id].state == FDIR_STATE_WARNING || 
        sensors_health[id].state == FDIR_STATE_RECOVERY) {
        sensors_health[id].state = FDIR_STATE_HEALTHY;
        sensors_health[id].recovery_count = 0;
    }
}

void FDIR_ReportFailure(void *sensor_id_ptr, int error_code) {
    SensorID_t id = (SensorID_t)(uintptr_t)sensor_id_ptr;
    if (id >= SENSOR_ID_COUNT) return;
    
    sensors_health[id].error_count++;
}

void FDIR_Update(void) {
    uint32_t now = HAL_GetTick();
    
    for(int i = 0; i < SENSOR_ID_COUNT; i++) {
        int16_t min_temp = sensor_temp_limits[i][0];
        int16_t max_temp = sensor_temp_limits[i][1];
        
        // ===== Temperature-based protection =====
        // Check if temperature is below operating minimum
        if (current_ext_temp_x100 < min_temp && !sensor_cold_disabled[i]) {
            // Disable sensor due to cold
            sensor_cold_disabled[i] = true;
            sensors_health[i].enabled = false;
            sensors_health[i].state = FDIR_STATE_WARNING; // Mark as warning, not permanent failure
            #ifdef DEBUG
            printf("FDIR: Sensor %d COLD DISABLED (%.1f°C < %.1f°C)\n", 
                   i, current_ext_temp_x100/100.0f, min_temp/100.0f);
            #endif
            
            // Physically disable if applicable
            if (i == SENSOR_ID_PMS) {
                #ifndef HOST_TEST_MODE
                HAL_GPIO_WritePin(PMS_SET_GPIO_Port, PMS_SET_Pin, GPIO_PIN_RESET);
                #endif
            }
            else if (i == SENSOR_ID_CO2) {
                // CM1107N has no enable pin, just stop reading
            }
            continue;
        }
        
        // Check for temperature recovery (with hysteresis)
        if (sensor_cold_disabled[i] && 
            current_ext_temp_x100 > (min_temp + TEMP_HYSTERESIS_X100)) {
            // Re-enable sensor
            sensor_cold_disabled[i] = false;
            sensors_health[i].enabled = true;
            sensors_health[i].recovery_count = 0;
            sensors_health[i].last_valid_update_ms = now;
            sensors_health[i].state = FDIR_STATE_RECOVERY;
            #ifdef DEBUG
            printf("FDIR: Sensor %d WARM RECOVERY (%.1f°C)\n", 
                   i, current_ext_temp_x100/100.0f);
            #endif
            
            // Physically re-enable
            if (i == SENSOR_ID_PMS) {
                #ifndef HOST_TEST_MODE
                HAL_GPIO_WritePin(PMS_SET_GPIO_Port, PMS_SET_Pin, GPIO_PIN_SET);
                #endif
            }
            
            Sensors_Reset((SensorID_t)i);
            continue;
        }
        
        // Skip disabled sensors
        if (!sensors_health[i].enabled) continue;
        if (sensors_health[i].state == FDIR_STATE_FAILURE_PERMANENT) continue;
        
        // ===== Timeout-based recovery =====
        uint32_t diff = now - sensors_health[i].last_valid_update_ms;
        uint32_t timeout = sensor_timeout_ms[i];
        
        if (diff > timeout) {
            sensors_health[i].state = FDIR_STATE_WARNING;
            
            #ifdef DEBUG
            printf("FDIR: Sensor %d Timeout (%lu ms). Recovery %lu/%d\n", 
                   i, diff, sensors_health[i].recovery_count + 1, sensor_max_recovery[i]);
            #endif
            
            Sensors_Reset((SensorID_t)i);
            
            sensors_health[i].last_valid_update_ms = now;
            sensors_health[i].recovery_count++;
            sensors_health[i].state = FDIR_STATE_RECOVERY;
            
            if (sensors_health[i].recovery_count >= sensor_max_recovery[i]) {
                sensors_health[i].state = FDIR_STATE_FAILURE_PERMANENT;
                #ifdef DEBUG
                printf("FDIR: Sensor %d PERMANENT FAILURE\n", i);
                #endif
            }
        }
    }
}

// Status query functions
FdirState_t FDIR_GetSensorState(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) return FDIR_STATE_FAILURE_PERMANENT;
    return sensors_health[id].state;
}

uint32_t FDIR_GetRecoveryCount(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) return 0;
    return sensors_health[id].recovery_count;
}

bool FDIR_IsSensorHealthy(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) return false;
    return sensors_health[id].state == FDIR_STATE_HEALTHY;
}

bool FDIR_IsSensorColdDisabled(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) return false;
    return sensor_cold_disabled[id];
}
