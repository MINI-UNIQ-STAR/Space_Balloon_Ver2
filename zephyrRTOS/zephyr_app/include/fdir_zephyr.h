/**
 * @file fdir_zephyr.h
 * @brief FDIR (Fault Detection, Isolation, Recovery) - Zephyr 포팅
 * @details 센서 상태 모니터링 및 자동 복구
 * @author Hyeonsu Park
 * @date 2026-02-22
 */

#ifndef FDIR_ZEPHYR_H
#define FDIR_ZEPHYR_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

/* ========================================================================== */
/* 타입 정의                                                                   */
/* ========================================================================== */

/** @brief FDIR 상태 열거형 */
typedef enum {
    FDIR_STATE_HEALTHY          = 0,    /**< 정상 동작 */
    FDIR_STATE_WARNING          = 1,    /**< 경고 상태 */
    FDIR_STATE_RECOVERY         = 2,    /**< 복구 시도 중 */
    FDIR_STATE_FAILURE_PERMANENT = 3    /**< 영구 고장 */
} FDIR_State_t;

/** @brief 센서 ID 열거형 */
typedef enum {
    SENSOR_ID_IMU = 0,
    SENSOR_ID_MAG,
    SENSOR_ID_BARO,
    SENSOR_ID_GPS,
    SENSOR_ID_PMS,
    SENSOR_ID_CO2,
    SENSOR_ID_SHT,
    SENSOR_ID_OZONE,
    SENSOR_ID_RAD,
    SENSOR_ID_COUNT
} SensorID_t;

/** @brief 센서 상태 구조체 */
typedef struct {
    FDIR_State_t state;                 /**< FDIR 상태 */
    uint32_t last_valid_update_ms;      /**< 마지막 유효 데이터 시간 */
    uint32_t recovery_count;            /**< 복구 시도 횟수 */
    bool cold_disabled;                 /**< 저온 비활성화 플래그 */
} SensorHealth_t;

/** @brief FDIR 상태 플래그 (텔레메트리용) */
typedef enum {
    FDIR_FLAG_SYS_OK        = (1 << 0),
    FDIR_FLAG_GPS_WARN      = (1 << 1),
    FDIR_FLAG_BARO_WARN     = (1 << 2),
    FDIR_FLAG_IMU_WARN      = (1 << 3),
    FDIR_FLAG_TEMP_WARN     = (1 << 4),
    FDIR_FLAG_HEATER_ACTIVE = (1 << 5),
    FDIR_FLAG_LOW_BATTERY   = (1 << 6),
    FDIR_FLAG_RECOVERY      = (1 << 7),
    FDIR_FLAG_ALT_JUMP      = (1 << 8),
    FDIR_FLAG_RANGE_ERROR   = (1 << 9)
} FDIR_StatusFlag_t;

/* ========================================================================== */
/* API 함수                                                                    */
/* ========================================================================== */

/**
 * @brief FDIR 시스템 초기화
 */
void FDIR_Init(void);

/**
 * @brief FDIR 주기적 업데이트 (메인 루프에서 호출)
 * @param now_ms 현재 시간 (ms)
 */
void FDIR_Update(uint32_t now_ms);

/**
 * @brief 센서 데이터 수신 보고
 * @param sensor_id 센서 ID
 * @param now_ms 현재 시간 (ms)
 */
void FDIR_ReportOK(SensorID_t sensor_id, uint32_t now_ms);

/**
 * @brief 센서 고장 보고
 * @param sensor_id 센서 ID
 * @param failure_type 고장 타입 (0=timeout, 1=range, 2=jump)
 */
void FDIR_ReportFailure(SensorID_t sensor_id, uint8_t failure_type);

/**
 * @brief FDIR 상태 플래그 조회
 * @return 상태 플래그 비트마스크
 */
uint16_t FDIR_GetStatusFlags(void);

/**
 * @brief 센서 상태 조회
 * @param sensor_id 센서 ID
 * @return 센서 상태 구조체 포인터
 */
const SensorHealth_t* FDIR_GetSensorHealth(SensorID_t sensor_id);

/**
 * @brief 저온 보호 활성화/비활성화
 * @param ext_temp_c_x100 외부 온도 (°C × 100)
 */
void FDIR_CheckTemperatureProtection(int16_t ext_temp_c_x100);

/**
 * @brief 범위 검증 - 기압
 * @param press_pa 기압 (Pa)
 * @return true=유효, false=범위 초과
 */
bool FDIR_ValidateRange_Baro(uint32_t press_pa);

/**
 * @brief 범위 검증 - GPS 고도
 * @param alt_m 고도 (m)
 * @return true=유효, false=범위 초과
 */
bool FDIR_ValidateRange_GPS_Alt(float alt_m);

/**
 * @brief 고도 점프 검출
 * @param new_alt_m 새 고도 (m)
 * @return true=유효, false=점프 검출
 */
bool FDIR_ValidateContinuity_GPS_Alt(float new_alt_m);

/**
 * @brief 백업 고도 계산
 * @return 백업 고도 (m), 유효하지 않으면 0
 */
float FDIR_GetBackupAltitude(void);

#endif /* FDIR_ZEPHYR_H */
