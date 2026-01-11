#pragma warning(disable:4819)
#include "fdir.h"
#include "main.h"
#include <stdio.h>


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
    [SENSOR_ID_IMU]      = {-4000, 8500},   // LSM6DSV16X: -40 degC ~ +85 degC
    [SENSOR_ID_MAG]      = {-4000, 8500},   // MLX90393: -40 degC ~ +85 degC
    [SENSOR_ID_BARO]     = {-4000, 8500},   // MS5611: -40 degC ~ +85 degC
    [SENSOR_ID_GPS]      = {-4000, 8500},   // XA1110: -40 degC ~ +85 degC
    [SENSOR_ID_PMS]      = {-1000, 6000},   // PMS3003: -10 degC ~ +60 degC *
    [SENSOR_ID_CO2]      = {-500,  5000},   // CM1107N: -5 degC ~ +50 degC *
    [SENSOR_ID_SHT]      = {-4000, 12500},  // SHT31: -40 degC ~ +125 degC
    [SENSOR_ID_RAD]      = {-2000, 6000},   // GDK101: -20 degC ~ +60 degC *
    [SENSOR_ID_EXT_TEMP] = {-9000, 25000},  // MCP9600 (K-Type): -90 degC ~ +250 degC (Stratosphere < -60)
};

// Temperature hysteresis (5°C = 500 in x100 scale)
#define TEMP_HYSTERESIS_X100  500

// Track cold-disabled state separately
static bool sensor_cold_disabled[SENSOR_ID_COUNT] = {false};

// Current external temperature (updated by FDIR_UpdateTemperature)
static int16_t current_ext_temp_x100 = 2500; // Default 25°C

void FDIR_Init(void) {
    uint8_t i;
    for (i = 0U; i < (uint8_t)SENSOR_ID_COUNT; i++) {
        sensors_health[i].last_valid_update_ms = HAL_GetTick();
        sensors_health[i].error_count = 0U;
        sensors_health[i].recovery_count = 0U;
        sensors_health[i].state = FDIR_STATE_HEALTHY;
        sensors_health[i].enabled = true;
        sensor_cold_disabled[i] = false;
    }
}

void FDIR_UpdateTemperature(int16_t ext_temp_c_x100) {
    current_ext_temp_x100 = ext_temp_c_x100;
}

void FDIR_ReportSuccess(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) {
        return;
    }
    
    sensors_health[id].last_valid_update_ms = HAL_GetTick();
    sensors_health[id].error_count = 0U;
    if ((sensors_health[id].state == FDIR_STATE_WARNING) || 
        (sensors_health[id].state == FDIR_STATE_RECOVERY)) {
        sensors_health[id].state = FDIR_STATE_HEALTHY;
        sensors_health[id].recovery_count = 0U;
    }
}

void FDIR_ReportFailure(SensorID_t id, int32_t error_code) {
    (void)error_code;  /* Currently unused, suppress warning */
    if (id >= SENSOR_ID_COUNT) {
        return;
    }
    
    sensors_health[id].error_count++;
}

void FDIR_Update(void) {
    uint32_t now = HAL_GetTick();
    uint8_t i;
    
    for (i = 0U; i < (uint8_t)SENSOR_ID_COUNT; i++) {
        int16_t min_temp = sensor_temp_limits[i][0];
        // int16_t max_temp = sensor_temp_limits[i][1]; // Unused
        
        // ===== Temperature-based protection =====
        // Check if temperature is below operating minimum
        if (current_ext_temp_x100 < min_temp && !sensor_cold_disabled[i]) {
            // Disable sensor due to cold
            sensor_cold_disabled[i] = true;
            sensors_health[i].enabled = false;
            sensors_health[i].state = FDIR_STATE_WARNING; // Mark as warning, not permanent failure
            #ifdef DEBUG
            printf("FDIR: Sensor %d COLD DISABLED (%.1f degC < %.1f degC)\n", 
                   i, current_ext_temp_x100/100.0f, min_temp/100.0f);
            #endif
            
            // Physically disable if applicable
            if (i == SENSOR_ID_PMS) {
                HAL_GPIO_WritePin(PMS_SET_GPIO_Port, PMS_SET_Pin, GPIO_PIN_RESET);
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
            printf("FDIR: Sensor %d WARM RECOVERY (%.1f degC)\n", 
                   i, current_ext_temp_x100/100.0f);
            #endif
            
            // Physically re-enable
            if (i == SENSOR_ID_PMS) {
                HAL_GPIO_WritePin(PMS_SET_GPIO_Port, PMS_SET_Pin, GPIO_PIN_SET);
            }
            
            Sensors_Reset((SensorID_t)i);
            continue;
        }
        
        /* Skip disabled sensors */
        if (sensors_health[i].enabled == false) {
            continue;
        }
        if (sensors_health[i].state == FDIR_STATE_FAILURE_PERMANENT) {
            continue;
        }
        
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
    if (id >= SENSOR_ID_COUNT) {
        return false;
    }
    return (sensors_health[id].state == FDIR_STATE_HEALTHY);
}

bool FDIR_IsSensorColdDisabled(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) return false;
    return sensor_cold_disabled[id];
}

// ===== New FDIR.md Compliance Functions =====

// Range limits (from FDIR.md L122-136)
#define BARO_MIN_PA     1000
#define BARO_MAX_PA     110000
#define GPS_ALT_MIN_M   (-500.0f)
#define GPS_ALT_MAX_M   50000.0f
#define TEMP_MIN_X100   (-8000)   // -80 degC
#define TEMP_MAX_X100   6000      // +60 degC

// Continuity check threshold (from FDIR.md L144)
#define ALT_JUMP_THRESHOLD_M  500.0f

// Altitude tracking for fallback and continuity
static float last_gps_alt_m = 0.0f;

static float current_gps_alt_m = 0.0f;
static float current_baro_alt_m = 0.0f;
static bool gps_alt_valid = false;
static bool baro_alt_valid = false;
static bool alt_jump_detected = false;
static bool range_error_detected = false;

bool FDIR_ValidateRange_Baro(uint32_t press_pa) {
    if (press_pa < BARO_MIN_PA || press_pa > BARO_MAX_PA) {
        range_error_detected = true;
        FDIR_ReportFailure(SENSOR_ID_BARO, 1);
        #ifdef DEBUG
        printf("FDIR: Baro range error: %lu Pa\n", press_pa);
        #endif
        return false;
    }
    return true;
}

bool FDIR_ValidateRange_GPS_Alt(float alt_m) {
    if (alt_m < GPS_ALT_MIN_M || alt_m > GPS_ALT_MAX_M) {
        range_error_detected = true;
        FDIR_ReportFailure(SENSOR_ID_GPS, 2);
        #ifdef DEBUG
        printf("FDIR: GPS altitude range error: %.1f m\n", alt_m);
        #endif
        return false;
    }
    return true;
}

bool FDIR_ValidateRange_Temp(int16_t temp_c_x100) {
    if (temp_c_x100 < TEMP_MIN_X100 || temp_c_x100 > TEMP_MAX_X100) {
        range_error_detected = true;
        #ifdef DEBUG
        printf("FDIR: Temp range error: %.1f C\n", temp_c_x100 / 100.0f);
        #endif
        return false;
    }
    return true;
}

bool FDIR_CheckContinuity_GPS_Alt(float new_alt_m) {
    static bool first_reading = true;
    
    if (first_reading) {
        last_gps_alt_m = new_alt_m;
        first_reading = false;
        return true;
    }
    
    float delta = new_alt_m - last_gps_alt_m;
    if (delta < 0) delta = -delta; // abs
    
    if (delta > ALT_JUMP_THRESHOLD_M) {
        alt_jump_detected = true;
        #ifdef DEBUG
        printf("FDIR: GPS altitude jump detected: %.1f -> %.1f (delta=%.1fm)\n", 
               last_gps_alt_m, new_alt_m, delta);
        #endif
        // Don't update last value on jump detection
        return false;
    }
    
    last_gps_alt_m = new_alt_m;
    alt_jump_detected = false;
    return true;
}

void FDIR_UpdateGPSAltitude(float gps_alt_m) {
    if (FDIR_ValidateRange_GPS_Alt(gps_alt_m) && FDIR_CheckContinuity_GPS_Alt(gps_alt_m)) {
        current_gps_alt_m = gps_alt_m;
        gps_alt_valid = true;
        FDIR_ReportSuccess(SENSOR_ID_GPS);
    } else {
        gps_alt_valid = false;
    }
}

void FDIR_UpdateBaroAltitude(float baro_alt_m) {
    // Convert altitude back to pressure for range check (simplified)
    // This is just for internal tracking, main range check should be on raw pressure
    current_baro_alt_m = baro_alt_m;
    baro_alt_valid = (sensors_health[SENSOR_ID_BARO].state == FDIR_STATE_HEALTHY);
}

float FDIR_GetBackupAltitude(void) {
    // Priority: Baro > GPS (baro is more accurate at high altitudes)
    // But if baro fails, use GPS as backup
    if (baro_alt_valid && sensors_health[SENSOR_ID_BARO].state == FDIR_STATE_HEALTHY) {
        return current_baro_alt_m;
    }
    
    if (gps_alt_valid && sensors_health[SENSOR_ID_GPS].state == FDIR_STATE_HEALTHY) {
        #ifdef DEBUG
        printf("FDIR: Using GPS altitude as backup: %.1f m\n", current_gps_alt_m);
        #endif
        return current_gps_alt_m;
    }
    
    // Both failed - return last known good value
    return current_baro_alt_m;
}

uint16_t FDIR_GetStatusFlags(void) {
    uint16_t flags = STATUS_SYS_OK;  // Start with OK
    
    // Check sensor states
    if (sensors_health[SENSOR_ID_GPS].state != FDIR_STATE_HEALTHY) {
        flags |= STATUS_GPS_WARN;
    }
    if (sensors_health[SENSOR_ID_BARO].state != FDIR_STATE_HEALTHY) {
        flags |= STATUS_BARO_WARN;
    }
    if (sensors_health[SENSOR_ID_IMU].state != FDIR_STATE_HEALTHY) {
        flags |= STATUS_IMU_WARN;
    }
    if (sensors_health[SENSOR_ID_SHT].state != FDIR_STATE_HEALTHY ||
        sensors_health[SENSOR_ID_EXT_TEMP].state != FDIR_STATE_HEALTHY) {
        flags |= STATUS_TEMP_WARN;
    }
    
    /* Check if any sensor is in recovery */
    uint8_t j;
    for (j = 0U; j < (uint8_t)SENSOR_ID_COUNT; j++) {
        if (sensors_health[j].state == FDIR_STATE_RECOVERY) {
            flags |= STATUS_FDIR_RECOVERY;
            break;
        }
    }
    
    // Altitude jump flag
    if (alt_jump_detected) {
        flags |= STATUS_ALT_JUMP;
    }
    
    // Range error flag
    if (range_error_detected) {
        flags |= STATUS_RANGE_ERROR;
        range_error_detected = false; // Reset after reading
    }
    
    // If any warning is set, clear SYS_OK
    if (flags & (STATUS_GPS_WARN | STATUS_BARO_WARN | STATUS_IMU_WARN | STATUS_TEMP_WARN)) {
        flags &= ~STATUS_SYS_OK;
    }
    
    return flags;
}

