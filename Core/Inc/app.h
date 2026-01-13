/**
 * @file app.h
 * @brief 메인 애플리케이션 헤더 - 성층권 풍선 센서 플랫폼
 * @details 50Hz 메인 루프, PID 히터 제어, 칼만 필터, FDIR 시스템 통합
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __APP_H
#define __APP_H

#include "main.h" // Holds HAL includes and basic types
#include "pid.h"
#include "kalman.h"
#include "telemetry.h"

/**
 * @defgroup APP_GLOBALS 전역 컨텍스트 (테스트/디버깅용 노출)
 * @{
 */

/** @brief 배터리 히터 PID 제어기 핸들 */
extern PID_HandleTypeDef hpid_bat;

/** @brief 보드 히터 PID 제어기 핸들 */
extern PID_HandleTypeDef hpid_brd;

/** @brief 칼만 필터 핸들 (고도 융합) */
extern KF_Handle_t hkf;

/** @brief 텔레메트리 프레임 버퍼 */
extern telemetry_frame_t telem_frame;

/** @brief 배터리 히터 제어 출력 (0.0 ~ 60.0%) */
extern float heater_battery_cmd;

/** @brief 보드 히터 제어 출력 (0.0 ~ 100.0%) */
extern float heater_board_cmd;

/** @} */ // end of APP_GLOBALS

/**
 * @defgroup APP_CONFIG 히터 전력 예산 보호 설정
 * @{
 */

/**
 * @brief 배터리 히터 최대 듀티 사이클 제한
 * @details Kapton 히터: 7.2W @ 5V = 1.44A
 *          60% 제한 → 평균 0.86A (전력 예산 보호)
 *          배터리 수명: 14,000mAh / 1,960mA × 1.12 = 8.0시간
 */
#define HEATER_BATT_MAX_DUTY  60.0f

/**
 * @brief 보드 히터 최대 듀티 사이클 제한
 * @details Minibulb: ~4W @ 5V = 0.8A
 *          하드웨어 테스트 후 제한 여부 결정 (현재 제한 없음)
 */
#define HEATER_BOARD_MAX_DUTY 100.0f

/** @} */ // end of APP_CONFIG

/**
 * @defgroup APP_LOWVOLT 저전압 보호
 * @{
 */

/**
 * @brief 저전압 모드 상태 플래그
 * @details 0 = 정상 모드, 1 = 저전압 모드 (히터 차단)
 *          진입: < 2.7V, 해제: > 2.9V (200mV 히스테리시스)
 */
extern uint8_t g_low_voltage_mode;

/**
 * @brief 저전압 모드 상태 조회
 * @return uint8_t 0 = 정상, 1 = 저전압 모드
 */
uint8_t App_IsLowVoltageMode(void);

/** @} */ // end of APP_LOWVOLT

/**
 * @defgroup APP_FUNCTIONS 메인 애플리케이션 함수
 * @{
 */

/**
 * @brief 애플리케이션 초기화
 * @details 초기화 순서:
 *          1. BSP_Init() - 보드 지원 패키지
 *          2. Sensors_Init() - 11개 센서 초기화
 *          3. PID_Init() - 히터 제어기 (Battery: Kp=400, Ki=6 / Board: Kp=500, Ki=5)
 *          4. KF_Init() - 칼만 필터 (dt=0.02s, Q=0.5, R=0.3)
 *          5. XCP_Init() - 캘리브레이션 프로토콜
 *          6. Actuators_Init() - PWM 히터
 *          7. FDIR_Init() - 고장 감지/복구
 *          8. PPS_Init() - GPS 1PPS 동기화
 * @note main.c의 main() 함수에서 1회 호출
 */
void App_Init(void);

/**
 * @brief 메인 루프 (50Hz, 20ms 주기)
 * @details 실행 순서:
 *          1. 센서 데이터 수집 (Sensors_Read_All, GPS, Battery, Temps, Radiation)
 *          2. FDIR 온도 업데이트 및 센서 상태 확인
 *          3. SHT31 히터 제어 (히스테리시스: 0°C ~ 2°C)
 *          4. GPS 고도 연속성 확인 (FDIR_CheckContinuity_GPS_Alt)
 *          5. IMU SFLP 데이터로 Roll/Pitch 계산 (쿼터니언 → Euler)
 *          6. 칼만 필터로 고도 추정 (KF_Predict + KF_Update_Altitude)
 *          7. PID 제어로 히터 듀티 사이클 계산 (목표: Battery=10°C, Board=5°C)
 *          8. 저전압 보호 체크 및 히터 제한
 *          9. 텔레메트리 프레임 구성 및 전송 (CRC16, UART3)
 *          10. XCP 측정값 업데이트
 *          11. FDIR 타임아웃 체크 및 복구 시도
 * @note main.c의 while(1) 루프에서 반복 호출
 *       실제 주기는 HAL_Delay(20) 또는 타이머 인터럽트로 제어
 */
void App_Loop(void);

/** @} */ // end of APP_FUNCTIONS

#endif
