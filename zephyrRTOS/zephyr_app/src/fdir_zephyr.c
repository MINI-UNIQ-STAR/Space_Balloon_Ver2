/**
 * @file fdir_zephyr.c
 * @brief FDIR (Fault Detection, Isolation, Recovery) - Zephyr RTOS 포팅
 * @details 센서 상태 모니터링 및 자동 복구
 * @author Hyeonsu Park
 * @date 2026-02-22
 */

#include "fdir_zephyr.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <inttypes.h>

LOG_MODULE_REGISTER(fdir, LOG_LEVEL_INF);

/* ========================================================================== */
/* 전역 변수 정의                                                              */
/* ========================================================================== */

/** @brief 센서 상태 배열 */
static SensorHealth_t sensors_health[SENSOR_ID_COUNT];

/** @brief 센서별 타임아웃 설정 (밀리초) */
static const uint32_t sensor_timeout_ms[SENSOR_ID_COUNT] = {
    [SENSOR_ID_IMU]      = 100,
    [SENSOR_ID_MAG]      = 500,
    [SENSOR_ID_BARO]     = 1000,
    [SENSOR_ID_GPS]      = 5000,
    [SENSOR_ID_PMS]      = 5000,
    [SENSOR_ID_CO2]      = 5000,
    [SENSOR_ID_SHT]      = 3000,
    [SENSOR_ID_OZONE]    = 3000,
    [SENSOR_ID_RAD]      = 3000,
};

/** @brief 센서별 최대 복구 시도 횟수 */
static const uint8_t sensor_max_recovery[SENSOR_ID_COUNT] = {
    [SENSOR_ID_IMU]      = 2,
    [SENSOR_ID_MAG]      = 2,
    [SENSOR_ID_BARO]     = 2,
    [SENSOR_ID_GPS]      = 3,
    [SENSOR_ID_PMS]      = 1,
    [SENSOR_ID_CO2]      = 1,
    [SENSOR_ID_SHT]      = 2,
    [SENSOR_ID_OZONE]    = 2,
    [SENSOR_ID_RAD]      = 2,
};

/** @brief 센서별 동작 온도 범위 (°C x 100) */
static const int16_t sensor_temp_limits[SENSOR_ID_COUNT][2] = {
    [SENSOR_ID_IMU]      = {-4000, 8500},
    [SENSOR_ID_MAG]      = {-4000, 8500},
    [SENSOR_ID_BARO]     = {-4000, 8500},
    [SENSOR_ID_GPS]      = {-4000, 8500},
    [SENSOR_ID_PMS]      = {-1000, 6000},
    [SENSOR_ID_CO2]      = {-500,  5000},
    [SENSOR_ID_SHT]      = {-4000, 12500},
    [SENSOR_ID_OZONE]    = {-2000, 6000},
    [SENSOR_ID_RAD]      = {-2000, 6000},
};

/** @brief 온도 히스테리시스 */
#define TEMP_HYSTERESIS_X100  500

/** @brief 현재 외부 온도 */
static int16_t current_ext_temp_x100 = 2500;

/** @brief 범위 제한 */
#define BARO_MIN_PA     1000
#define BARO_MAX_PA     110000
#define GPS_ALT_MIN_M   (-500.0f)
#define GPS_ALT_MAX_M   50000.0f
#define ALT_JUMP_THRESHOLD_M  500.0f

/** @brief 고도 추적 변수 */
static float last_gps_alt_m = 0.0f;
static float current_gps_alt_m = 0.0f;
static float current_baro_alt_m = 0.0f;
static bool gps_alt_valid = false;
static bool baro_alt_valid = false;
static bool alt_jump_detected = false;
static bool range_error_detected = false;

/* ========================================================================== */
/* API 구현                                                                    */
/* ========================================================================== */

void FDIR_Init(void) {
    for (int i = 0; i < SENSOR_ID_COUNT; i++) {
        sensors_health[i].last_valid_update_ms = k_uptime_get_32();
        sensors_health[i].recovery_count = 0;
        sensors_health[i].state = FDIR_STATE_HEALTHY;
        sensors_health[i].cold_disabled = false;
    }
    LOG_INF("FDIR initialized");
}

void FDIR_Update(uint32_t now_ms) {
    for (int i = 0; i < SENSOR_ID_COUNT; i++) {
        int16_t min_temp = sensor_temp_limits[i][0];
        
        /* 저온 보호 */
        if (current_ext_temp_x100 < min_temp && !sensors_health[i].cold_disabled) {
            sensors_health[i].cold_disabled = true;
            sensors_health[i].state = FDIR_STATE_WARNING;
            LOG_WRN("Sensor %d cold disabled (%.1f C < %.1f C)", 
                    i, current_ext_temp_x100/100.0f, min_temp/100.0f);
            continue;
        }
        
        /* 저온 복구 */
        if (sensors_health[i].cold_disabled && 
            current_ext_temp_x100 > (min_temp + TEMP_HYSTERESIS_X100)) {
            sensors_health[i].cold_disabled = false;
            sensors_health[i].recovery_count = 0;
            sensors_health[i].last_valid_update_ms = now_ms;
            sensors_health[i].state = FDIR_STATE_RECOVERY;
            LOG_INF("Sensor %d warm recovery (%.1f C)", i, current_ext_temp_x100/100.0f);
            continue;
        }
        
        /* 영구 고장 센서 스킵 */
        if (sensors_health[i].state == FDIR_STATE_FAILURE_PERMANENT) {
            continue;
        }
        
        /* 타임아웃 검사 */
        uint32_t diff = now_ms - sensors_health[i].last_valid_update_ms;
        uint32_t timeout = sensor_timeout_ms[i];
        
        if (diff > timeout) {
            sensors_health[i].state = FDIR_STATE_WARNING;
            
            LOG_WRN("Sensor %d timeout (%" PRIu32 " ms). Recovery %u/%u",
                    i, diff, sensors_health[i].recovery_count + 1, sensor_max_recovery[i]);
            
            sensors_health[i].last_valid_update_ms = now_ms;
            sensors_health[i].recovery_count++;
            sensors_health[i].state = FDIR_STATE_RECOVERY;
            
            if (sensors_health[i].recovery_count >= sensor_max_recovery[i]) {
                sensors_health[i].state = FDIR_STATE_FAILURE_PERMANENT;
                LOG_ERR("Sensor %d PERMANENT FAILURE", i);
            }
        }
    }
}

void FDIR_ReportOK(SensorID_t sensor_id, uint32_t now_ms) {
    if (sensor_id >= SENSOR_ID_COUNT) return;
    
    sensors_health[sensor_id].last_valid_update_ms = now_ms;
    if (sensors_health[sensor_id].state == FDIR_STATE_WARNING ||
        sensors_health[sensor_id].state == FDIR_STATE_RECOVERY) {
        sensors_health[sensor_id].state = FDIR_STATE_HEALTHY;
        sensors_health[sensor_id].recovery_count = 0;
        LOG_INF("Sensor %d recovered", sensor_id);
    }
}

void FDIR_ReportFailure(SensorID_t sensor_id, uint8_t failure_type) {
    if (sensor_id >= SENSOR_ID_COUNT) return;
    (void)failure_type;
    
    LOG_WRN("Sensor %d failure reported (type %u)", sensor_id, failure_type);
}

void FDIR_CheckTemperatureProtection(int16_t ext_temp_c_x100) {
    current_ext_temp_x100 = ext_temp_c_x100;
}

uint16_t FDIR_GetStatusFlags(void) {
    uint16_t flags = FDIR_FLAG_SYS_OK;
    
    if (sensors_health[SENSOR_ID_GPS].state != FDIR_STATE_HEALTHY) {
        flags |= FDIR_FLAG_GPS_WARN;
    }
    if (sensors_health[SENSOR_ID_BARO].state != FDIR_STATE_HEALTHY) {
        flags |= FDIR_FLAG_BARO_WARN;
    }
    if (sensors_health[SENSOR_ID_IMU].state != FDIR_STATE_HEALTHY) {
        flags |= FDIR_FLAG_IMU_WARN;
    }
    if (sensors_health[SENSOR_ID_SHT].state != FDIR_STATE_HEALTHY) {
        flags |= FDIR_FLAG_TEMP_WARN;
    }
    
    for (int i = 0; i < SENSOR_ID_COUNT; i++) {
        if (sensors_health[i].state == FDIR_STATE_RECOVERY) {
            flags |= FDIR_FLAG_RECOVERY;
            break;
        }
    }
    
    if (alt_jump_detected) flags |= FDIR_FLAG_ALT_JUMP;
    if (range_error_detected) flags |= FDIR_FLAG_RANGE_ERROR;
    
    return flags;
}

const SensorHealth_t* FDIR_GetSensorHealth(SensorID_t sensor_id) {
    if (sensor_id >= SENSOR_ID_COUNT) return NULL;
    return &sensors_health[sensor_id];
}

bool FDIR_ValidateRange_Baro(uint32_t press_pa) {
    if (press_pa < BARO_MIN_PA || press_pa > BARO_MAX_PA) {
        range_error_detected = true;
        LOG_WRN("Baro range error: %" PRIu32 " Pa", press_pa);
        return false;
    }
    return true;
}

bool FDIR_ValidateRange_GPS_Alt(float alt_m) {
    if (alt_m < GPS_ALT_MIN_M || alt_m > GPS_ALT_MAX_M) {
        range_error_detected = true;
        LOG_WRN("GPS altitude range error: %.1f m", alt_m);
        return false;
    }
    return true;
}

bool FDIR_ValidateContinuity_GPS_Alt(float new_alt_m) {
    static bool first_reading = true;
    
    if (first_reading) {
        last_gps_alt_m = new_alt_m;
        first_reading = false;
        return true;
    }
    
    float delta = new_alt_m > last_gps_alt_m ? 
                  (new_alt_m - last_gps_alt_m) : 
                  (last_gps_alt_m - new_alt_m);
    
    if (delta > ALT_JUMP_THRESHOLD_M) {
        alt_jump_detected = true;
        LOG_WRN("GPS altitude jump: %.1f -> %.1f (delta=%.1f m)",
                last_gps_alt_m, new_alt_m, delta);
        return false;
    }
    
    last_gps_alt_m = new_alt_m;
    alt_jump_detected = false;
    return true;
}

float FDIR_GetBackupAltitude(void) {
    if (baro_alt_valid && 
        sensors_health[SENSOR_ID_BARO].state == FDIR_STATE_HEALTHY) {
        return current_baro_alt_m;
    }
    
    if (gps_alt_valid && 
        sensors_health[SENSOR_ID_GPS].state == FDIR_STATE_HEALTHY) {
        LOG_INF("Using GPS altitude as backup: %.1f m", current_gps_alt_m);
        return current_gps_alt_m;
    }
    
    return current_baro_alt_m;
}
