/**
 * @file fdir.h
 * @brief FDIR 시스템 인터페이스 - 고장 감지, 격리 및 복구 (Fault Detection, Isolation and Recovery)
 * @details 타임아웃 기반 센서 건강 모니터링, 온도 기반 보호, 자동 복구 메커니즘
 *          복구 단계: 소프트 재초기화 → I2C 버스 복구 → GPIO 하드웨어 리셋
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __FDIR_H
#define __FDIR_H

#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"

/**
 * @brief FDIR 상태 머신
 * @details 상태 전이: HEALTHY → WARNING → RECOVERY → FAILURE_PERMANENT
 */
typedef enum {
    FDIR_STATE_HEALTHY = 0,          /**< 센서 정상 동작 (에러 없음) */
    FDIR_STATE_WARNING,              /**< 경고 상태 (일시적 통신 실패) */
    FDIR_STATE_RECOVERY,             /**< 복구 시도 중 (I2C 버스/GPIO 리셋) */
    FDIR_STATE_FAILURE_PERMANENT     /**< 영구 실패 (최대 복구 시도 초과) */
} FdirState_t;

/**
 * @brief 센서별 건강 상태 추적 구조체
 */
typedef struct {
    uint32_t last_valid_update_ms;   /**< 마지막 정상 업데이트 시각 (HAL_GetTick) */
    uint32_t error_count;            /**< 누적 에러 카운트 */
    uint32_t recovery_count;         /**< 복구 시도 횟수 */
    FdirState_t state;               /**< 현재 FDIR 상태 */
    bool enabled;                    /**< 센서 활성화 여부 (온도 보호 시 false) */
} SensorHealth_t;

/**
 * @brief 시스템 전체 건강 상태 (디버깅/텔레메트리용)
 * @note 실제 구현은 fdir.c의 내부 배열 사용 (sensors_health[SENSOR_ID_COUNT])
 */
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

/**
 * @defgroup FDIR_CORE 핵심 FDIR 함수
 * @{
 */

/**
 * @brief FDIR 시스템 초기화
 * @details 모든 센서 상태를 HEALTHY로 초기화, 타임스탬프 설정
 * @note App_Init()에서 1회 호출
 */
void FDIR_Init(void);

/**
 * @brief FDIR 업데이트 (타임아웃 및 복구 수행)
 * @details 실행 순서:
 *          1. 각 센서의 타임아웃 체크 (last_valid_update_ms 비교)
 *          2. 타임아웃 발생 시 WARNING → RECOVERY 상태 전이
 *          3. 복구 시도: Sensors_Reset(id) 호출 (3단계 복구)
 *          4. 최대 복구 시도 초과 시 FAILURE_PERMANENT
 *          5. 온도 기반 센서 비활성화/재활성화 (히스테리시스 5°C)
 * @note App_Loop()에서 매 주기 호출 (50Hz)
 */
void FDIR_Update(void);

/**
 * @brief FDIR 초기화 (Blocking Reset State Machine)
 * @details App_Loop에서 호출하여 비동기 리셋 처리
 */
void FDIR_Process(void);


/**
 * @brief 센서 정상 동작 보고
 * @param[in] sensor_id 센서 ID
 * @details Sensors_Read_XXX() 함수에서 성공 시 호출
 *          타임스탬프 업데이트, 에러 카운트 초기화
 */
void FDIR_ReportSuccess(SensorID_t sensor_id);

/**
 * @brief 센서 오류 보고
 * @param[in] sensor_id 센서 ID
 * @param[in] error_code HAL 에러 코드 (HAL_I2C_ERROR, HAL_UART_ERROR 등)
 * @details Sensors_Read_XXX() 함수에서 실패 시 호출
 *          에러 카운트 증가, 상태 전이 트리거
 */
void FDIR_ReportFailure(SensorID_t sensor_id, int32_t error_code);

/** @} */ // end of FDIR_CORE

/**
 * @defgroup FDIR_STATUS 상태 조회 함수
 * @{
 */

/**
 * @brief 센서 FDIR 상태 조회
 * @param[in] id 센서 ID
 * @return FdirState_t 현재 FDIR 상태
 */
FdirState_t FDIR_GetSensorState(SensorID_t id);

/**
 * @brief 센서 복구 시도 횟수 조회
 * @param[in] id 센서 ID
 * @return uint32_t 누적 복구 시도 횟수
 */
uint32_t FDIR_GetRecoveryCount(SensorID_t id);

/**
 * @brief 센서 건강 상태 확인
 * @param[in] id 센서 ID
 * @return bool true=건강, false=오류/실패
 */
bool FDIR_IsSensorHealthy(SensorID_t id);

/**
 * @brief 센서 저온 비활성화 상태 확인
 * @param[in] id 센서 ID
 * @return bool true=저온 비활성화, false=정상 동작 온도
 * @details 비활성화 조건 (온도 임계값, fdir.c):
 *          - PMS3003: < -10°C
 *          - CM1107N: < -5°C
 *          - GDK101: < -20°C
 *          재활성화: 각 min_temp + 5°C (히스테리시스)
 */
bool FDIR_IsSensorColdDisabled(SensorID_t id);

/** @} */ // end of FDIR_STATUS

/**
 * @defgroup FDIR_THERMAL 온도 기반 보호
 * @{
 */

/**
 * @brief 외부 온도 업데이트 (온도 보호 로직용)
 * @param[in] ext_temp_c_x100 외부 온도 (°C × 100, MCP9600)
 * @details FDIR_Update()에서 센서별 동작 온도 범위 체크
 *          범위 벗어나면 센서 비활성화 (enabled = false)
 * @note App_Loop()에서 매 주기 호출
 */
void FDIR_UpdateTemperature(int16_t ext_temp_c_x100);

/** @} */ // end of FDIR_THERMAL

/**
 * @defgroup FDIR_VALIDATION 데이터 유효성 검사
 * @{
 */

/**
 * @brief 기압 범위 검증
 * @param[in] press_pa 기압 (Pa)
 * @return bool true=정상 범위, false=범위 초과
 * @details 정상 범위: 1,000 ~ 110,000 Pa (10 ~ 1100 mbar)
 *          MS5611 사양: 10 ~ 1200 mbar
 */
bool FDIR_ValidateRange_Baro(uint32_t press_pa);

/**
 * @brief GPS 고도 범위 검증
 * @param[in] alt_m GPS 고도 (m)
 * @return bool true=정상 범위, false=범위 초과
 * @details 정상 범위: -500 ~ 50,000 m
 *          성층권 풍선: ~30,000 m (최대 고도)
 */
bool FDIR_ValidateRange_GPS_Alt(float alt_m);

/**
 * @brief 온도 범위 검증
 * @param[in] temp_c_x100 온도 (°C × 100)
 * @return bool true=정상 범위, false=범위 초과
 * @details 정상 범위: -9000 ~ 12500 (°C × 100)
 *          즉, -90°C ~ +125°C
 */
bool FDIR_ValidateRange_Temp(int16_t temp_c_x100);

/** @} */ // end of FDIR_VALIDATION

/**
 * @defgroup FDIR_CONTINUITY 연속성 검사
 * @{
 */

/**
 * @brief GPS 고도 연속성 체크 (급격한 고도 점프 감지)
 * @param[in] new_alt_m 새로운 GPS 고도 (m)
 * @return bool true=연속적, false=고도 점프 감지
 * @details 점프 감지 임계값: 500m (20ms 주기 기준)
 *          점프 감지 시 STATUS_ALT_JUMP 플래그 설정
 * @note App_Loop()에서 GPS 데이터 수집 후 호출
 */
bool FDIR_CheckContinuity_GPS_Alt(float new_alt_m);

/** @} */ // end of FDIR_CONTINUITY

/**
 * @defgroup FDIR_ALTITUDE 백업 고도 관리
 * @{
 */

/**
 * @brief 백업 고도 조회 (GPS 실패 시 기압 고도 사용)
 * @return float 백업 고도 (m)
 * @details 우선순위: 1. GPS 고도 (가용 시) → 2. 기압 고도
 */
float FDIR_GetBackupAltitude(void);

/**
 * @brief GPS 고도 업데이트
 * @param[in] gps_alt_m GPS 고도 (m)
 * @details 내부 백업 고도 저장소 업데이트
 */
void FDIR_UpdateGPSAltitude(float gps_alt_m);

/**
 * @brief 기압 고도 업데이트
 * @param[in] baro_alt_m 기압 고도 (m)
 * @details 내부 백업 고도 저장소 업데이트
 */
void FDIR_UpdateBaroAltitude(float baro_alt_m);

/** @} */ // end of FDIR_ALTITUDE

/**
 * @defgroup FDIR_TELEMETRY 텔레메트리 상태 플래그
 * @{
 */

/**
 * @brief 시스템 상태 플래그 생성 (텔레메트리 전송용)
 * @return uint16_t 상태 플래그 비트마스크
 * @details 플래그 비트 구성 (각 비트별 의미):
 *          - bit 0: STATUS_SYS_OK (모든 센서 정상)
 *          - bit 1: STATUS_GPS_WARN (GPS 경고)
 *          - bit 2: STATUS_BARO_WARN (기압계 경고)
 *          - bit 3: STATUS_IMU_WARN (IMU 경고)
 *          - bit 4: STATUS_TEMP_WARN (온도 이상)
 *          - bit 5: STATUS_HEATER_ACTIVE (히터 작동 중)
 *          - bit 6: STATUS_LOW_BATTERY (저전압)
 *          - bit 7: STATUS_FDIR_RECOVERY (복구 진행 중)
 *          - bit 8: STATUS_ALT_JUMP (고도 점프 감지)
 *          - bit 9: STATUS_RANGE_ERROR (범위 오류)
 * @note App_Loop()에서 텔레메트리 전송 전 호출
 */
uint16_t FDIR_GetStatusFlags(void);

/** @} */ // end of FDIR_TELEMETRY

/**
 * @defgroup FDIR_FLAGS 상태 플래그 정의
 * @{
 */

#define STATUS_SYS_OK         (1 << 0)  /**< 시스템 정상 (모든 센서 HEALTHY) */
#define STATUS_GPS_WARN       (1 << 1)  /**< GPS 경고/실패 */
#define STATUS_BARO_WARN      (1 << 2)  /**< 기압계 경고/실패 */
#define STATUS_IMU_WARN       (1 << 3)  /**< IMU 경고/실패 */
#define STATUS_TEMP_WARN      (1 << 4)  /**< 온도 이상 (과열/저온) */
#define STATUS_HEATER_ACTIVE  (1 << 5)  /**< 히터 활성화 (Duty > 10%) */
#define STATUS_LOW_BATTERY    (1 << 6)  /**< 저전압 모드 (< 2.7V) */
#define STATUS_FDIR_RECOVERY  (1 << 7)  /**< FDIR 복구 진행 중 */
#define STATUS_ALT_JUMP       (1 << 8)  /**< 고도 급변 감지 (> 500m) */
#define STATUS_RANGE_ERROR    (1 << 9)  /**< 센서 데이터 범위 초과 */

/** @} */ // end of FDIR_FLAGS

#endif
