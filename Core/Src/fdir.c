/**
 * @file fdir.c
 * @brief 고장 검출, 격리 및 복구 (FDIR) 시스템 구현
 * @details 센서 상태 모니터링 및 자동 복구
 *          - 타임아웃 기반 고장 검출
 *          - 온도 기반 센서 비활성화/재활성화
 *          - 범위 및 연속성 검증
 *          - 자동 센서 리셋 및 I2C 버스 복구
 *          - 백업 고도 계산 (BARO/GPS 페일오버)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "fdir.h"
#include "main.h"
#include <stdio.h>
#include <inttypes.h>  /* MISRA C:2023 - PRIu32 for portable printf */

/* ========================================================================== */
/* 전역 변수 정의                                                              */
/* ========================================================================== */

/** @brief 센서 상태 배열 (9개 센서) */
static SensorHealth_t sensors_health[SENSOR_ID_COUNT];

/** @brief 센서별 타임아웃 설정 (밀리초) */
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
    [SENSOR_ID_TEMP_BAT] = 3000,   // 1Hz → 3s timeout
    [SENSOR_ID_TEMP_BOARD] = 3000, // 1Hz → 3s timeout
};

/** @brief 센서별 최대 복구 시도 횟수 */
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
    [SENSOR_ID_TEMP_BAT] = 1,
    [SENSOR_ID_TEMP_BOARD] = 1,
};

/**
 * @brief 센서별 동작 온도 범위 (°C x 100)
 * @details 형식: {min_temp, max_temp}
 *          온도 범위를 벗어나면 센서 비활성화
 */
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
    [SENSOR_ID_TEMP_BAT] = {-4000, 8500},   // DS18B20
    [SENSOR_ID_TEMP_BOARD] = {-4000, 8500}, // DS18B20
};

/** @brief 온도 히스테리시스 (5°C = 500 x 100) */
#define TEMP_HYSTERESIS_X100  500

/** @brief 저온 비활성화 상태 플래그 */
static bool sensor_cold_disabled[SENSOR_ID_COUNT] = {false};

/** @brief 현재 외부 온도 (°C x 100, 기본값: 25°C) */
static int16_t current_ext_temp_x100 = 2500;

/**
 * @brief FDIR 시스템 초기화
 * @details 모든 센서 상태를 HEALTHY로 초기화
 *          - 마지막 업데이트 시간: 현재 틱
 *          - 오류 카운터: 0
 *          - 복구 카운터: 0
 *          - 활성화 상태: true
 */
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

/**
 * @brief 외부 온도 업데이트
 * @param ext_temp_c_x100 외부 온도 (°C x 100)
 * @details 온도 기반 센서 보호에 사용
 */
void FDIR_UpdateTemperature(int16_t ext_temp_c_x100) {
    current_ext_temp_x100 = ext_temp_c_x100;
}

/**
 * @brief 센서 정상 작동 보고
 * @param id 센서 ID
 * @details 센서가 정상 데이터를 반환했을 때 호출
 *          - 마지막 업데이트 시간 갱신
 *          - 오류 카운터 리셋
 *          - WARNING/RECOVERY 상태에서 HEALTHY로 전환
 */
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

/**
 * @brief 센서 고장 보고
 * @param id 센서 ID
 * @param error_code 오류 코드 (현재 미사용)
 * @details 센서 읽기 실패 시 오류 카운터 증가
 */
void FDIR_ReportFailure(SensorID_t id, int32_t error_code) {
    (void)error_code;  /* Currently unused, suppress warning */
    if (id >= SENSOR_ID_COUNT) {
        return;
    }
    
    sensors_health[id].error_count++;
}

/**
 * @brief FDIR 주기적 업데이트 (50Hz)
 * @details 실행 순서:
 *          1. 온도 기반 센서 보호 (저온 비활성화/재활성화)
 *          2. 타임아웃 기반 고장 검출
 *          3. 자동 복구 시도 (센서 리셋)
 *          4. 영구 고장 판정 (최대 복구 횟수 초과)
 * @note App_Loop()에서 매 사이클 호출됨
 */
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
            printf("FDIR: Sensor %d Timeout (%" PRIu32 " ms). Recovery %" PRIu32 "/%d\n", 
                   i, diff, sensors_health[i].recovery_count + 1U, sensor_max_recovery[i]);
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

/**
 * @brief FDIR 처리 (Non-blocking Reset State Machine)
 * @details App_Loop에서 호출하여 센서 리셋 상태 머신을 구동
 */
void FDIR_Process(void) {
    Sensors_ProcessReset();
}

/* ========================================================================== */
/* 상태 조회 함수                                                             */
/* ========================================================================== */

/**
 * @brief 센서 상태 조회
 * @param id 센서 ID
 * @return FdirState_t 센서 상태 (HEALTHY/WARNING/RECOVERY/FAILURE_PERMANENT)
 */
FdirState_t FDIR_GetSensorState(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) {
        return FDIR_STATE_FAILURE_PERMANENT;
    }
    return sensors_health[id].state;
}

/**
 * @brief 복구 시도 횟수 조회
 * @param id 센서 ID
 * @return uint32_t 복구 시도 횟수
 */
uint32_t FDIR_GetRecoveryCount(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) {
        return 0;
    }
    return sensors_health[id].recovery_count;
}

/**
 * @brief 센서 정상 상태 확인
 * @param id 센서 ID
 * @return bool true = 정상, false = 고장/경고
 */
bool FDIR_IsSensorHealthy(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) {
        return false;
    }
    return (sensors_health[id].state == FDIR_STATE_HEALTHY);
}

/**
 * @brief 센서 저온 비활성화 상태 확인
 * @param id 센서 ID
 * @return bool true = 저온으로 비활성화됨, false = 정상
 */
bool FDIR_IsSensorColdDisabled(SensorID_t id) {
    if (id >= SENSOR_ID_COUNT) {
        return false;
    }
    return sensor_cold_disabled[id];
}

/* ========================================================================== */
/* 범위 검증 및 연속성 체크                                                    */
/* ========================================================================== */

/** @brief 기압 센서 범위 제한 (Pa) */
#define BARO_MIN_PA     1000
#define BARO_MAX_PA     110000

/** @brief GPS 고도 범위 제한 (m) */
#define GPS_ALT_MIN_M   (-500.0f)
#define GPS_ALT_MAX_M   50000.0f

/** @brief 온도 범위 제한 (°C x 100) */
#define TEMP_MIN_X100   (-8000)   // -80°C
#define TEMP_MAX_X100   6000      // +60°C

/** @brief 고도 점프 감지 임계값 (m) */
#define ALT_JUMP_THRESHOLD_M  500.0f

/** @brief 이전 GPS 고도 (연속성 체크용) */
static float32_t last_gps_alt_m = 0.0f;

/** @brief 현재 GPS 고도 */
static float32_t current_gps_alt_m = 0.0f;

/** @brief 현재 기압 고도 */
static float32_t current_baro_alt_m = 0.0f;

/** @brief GPS 고도 유효성 플래그 */
static bool gps_alt_valid = false;

/** @brief 기압 고도 유효성 플래그 */
static bool baro_alt_valid = false;

/** @brief 고도 점프 검출 플래그 */
static bool alt_jump_detected = false;

/** @brief 범위 오류 검출 플래그 */
static bool range_error_detected = false;

/**
 * @brief 기압 센서 범위 검증
 * @param press_pa 기압 (Pa)
 * @return bool true = 정상 범위, false = 범위 초과
 * @details 범위: 1000 Pa ~ 110000 Pa (해발 -500m ~ 50km)
 */
bool FDIR_ValidateRange_Baro(uint32_t press_pa) {
    if (press_pa < BARO_MIN_PA || press_pa > BARO_MAX_PA) {
        range_error_detected = true;
        FDIR_ReportFailure(SENSOR_ID_BARO, 1);
        #ifdef DEBUG
        printf("FDIR: Baro range error: %" PRIu32 " Pa\n", press_pa);
        #endif
        return false;
    }
    return true;
}

/**
 * @brief GPS 고도 범위 검증
 * @param alt_m GPS 고도 (m)
 * @return bool true = 정상 범위, false = 범위 초과
 * @details 범위: -500m ~ 50000m
 */
bool FDIR_ValidateRange_GPS_Alt(float32_t alt_m) {
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

/**
 * @brief 온도 범위 검증
 * @param temp_c_x100 온도 (°C x 100)
 * @return bool true = 정상 범위, false = 범위 초과
 * @details 범위: -80°C ~ +60°C
 */
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

/**
 * @brief GPS 고도 연속성 체크
 * @param new_alt_m 새로운 GPS 고도 (m)
 * @return bool true = 연속적, false = 점프 검출
 * @details 이전 고도와 500m 이상 차이나면 점프로 판정
 */
bool FDIR_CheckContinuity_GPS_Alt(float32_t new_alt_m) {
    static bool first_reading = true;
    
    if (first_reading) {
        last_gps_alt_m = new_alt_m;
        first_reading = false;
        return true;
    }
    
    float32_t delta = new_alt_m - last_gps_alt_m;
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

/**
 * @brief GPS 고도 업데이트 (범위 및 연속성 검증 포함)
 * @param gps_alt_m GPS 고도 (m)
 * @details 범위 및 연속성 검증 통과 시에만 고도 업데이트
 */
void FDIR_UpdateGPSAltitude(float32_t gps_alt_m) {
    if (FDIR_ValidateRange_GPS_Alt(gps_alt_m) && FDIR_CheckContinuity_GPS_Alt(gps_alt_m)) {
        current_gps_alt_m = gps_alt_m;
        gps_alt_valid = true;
        FDIR_ReportSuccess(SENSOR_ID_GPS);
    } else {
        gps_alt_valid = false;
    }
}

/**
 * @brief 기압 고도 업데이트
 * @param baro_alt_m 기압 고도 (m)
 * @details 기압 센서 상태에 따라 유효성 플래그 설정
 */
void FDIR_UpdateBaroAltitude(float32_t baro_alt_m) {
    // Convert altitude back to pressure for range check (simplified)
    // This is just for internal tracking, main range check should be on raw pressure
    current_baro_alt_m = baro_alt_m;
    baro_alt_valid = (sensors_health[SENSOR_ID_BARO].state == FDIR_STATE_HEALTHY);
}

/**
 * @brief 백업 고도 조회 (BARO/GPS 페일오버)
 * @return float32_t 백업 고도 (m)
 * @details 우선순위: BARO > GPS
 *          - BARO 정상: 기압 고도 반환
 *          - BARO 고장, GPS 정상: GPS 고도 반환
 *          - 둘 다 고장: 마지막 기압 고도 반환
 */
float32_t FDIR_GetBackupAltitude(void) {
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

/**
 * @brief 시스템 상태 플래그 생성 (텔레메트리용)
 * @return uint16_t 상태 플래그 비트마스크
 * @details 비트 플래그:
 *          - STATUS_SYS_OK: 시스템 정상
 *          - STATUS_GPS_WARN: GPS 경고
 *          - STATUS_BARO_WARN: 기압계 경고
 *          - STATUS_IMU_WARN: IMU 경고
 *          - STATUS_TEMP_WARN: 온도 센서 경고
 *          - STATUS_FDIR_RECOVERY: 복구 중
 *          - STATUS_ALT_JUMP: 고도 점프 검출
 *          - STATUS_RANGE_ERROR: 범위 오류 검출
 */
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

