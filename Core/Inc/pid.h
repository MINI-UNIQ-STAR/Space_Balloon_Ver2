/**
 * @file pid.h
 * @brief PID 제어기 인터페이스 - 히터 온도 제어용
 * @details 비례-적분-미분 제어기 구현
 *          배터리 히터 (Kapton): 목표 10°C, Kp=400, Ki=6, Max=60%
 *          보드 히터 (Minibulb): 목표 5°C, Kp=500, Ki=5, Max=100%
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief PID 제어기 핸들 구조체
 * @details Anti-windup 기능 포함 (MaxOutput으로 적분항 포화 방지)
 */
typedef struct {
    float Kp;                /**< 비례 게인 (Proportional Gain) */
    float Ki;                /**< 적분 게인 (Integral Gain) */
    float Kd;                /**< 미분 게인 (Derivative Gain) */
    float MaxOutput;         /**< 최대 출력 제한 (0.0 ~ MaxOutput) */

    float Target;            /**< 목표값 (Setpoint, °C) */
    float IntegratedError;   /**< 누적 오차 (Integral Error) */
    float LastError;         /**< 이전 오차 (미분 계산용) */
} PID_HandleTypeDef;

/**
 * @defgroup PID_FUNCTIONS PID 제어 함수
 * @{
 */

/**
 * @brief PID 제어기 초기화
 * @param[in,out] hpid PID 제어기 핸들 포인터
 * @param[in] Kp 비례 게인
 * @param[in] Ki 적분 게인
 * @param[in] Kd 미분 게인
 * @param[in] MaxOutput 최대 출력 (예: 60.0 = 60%)
 * @details 권장 설정 (app.c):
 *          - 배터리 히터: PID_Init(&hpid_bat, 400.0f, 6.0f, 0.0f, 60.0f)
 *          - 보드 히터: PID_Init(&hpid_brd, 500.0f, 5.0f, 0.0f, 100.0f)
 * @note App_Init()에서 1회 호출, 이후 hpid->Target 설정
 */
void PID_Init(PID_HandleTypeDef *hpid, float Kp, float Ki, float Kd, float MaxOutput);

/**
 * @brief PID 제어 업데이트 (제어 출력 계산)
 * @param[in,out] hpid PID 제어기 핸들 포인터
 * @param[in] measurement 현재 측정값 (°C, DS18B20 온도)
 * @param[in] dt 샘플링 주기 (초, 예: 0.02s = 50Hz)
 * @return float 제어 출력 (0.0 ~ MaxOutput)
 * @details PID 알고리즘:
 *          - error = Target - measurement
 *          - P_term = Kp × error
 *          - I_term = Ki × ∫error dt (Anti-windup: 포화 시 적분 중단)
 *          - D_term = Kd × (error - LastError) / dt
 *          - output = P_term + I_term + D_term
 *          - output 제한: [0, MaxOutput]
 * @note App_Loop()에서 매 주기 호출 (50Hz)
 *       출력값은 Actuators_SetHeater_XXX()로 PWM 듀티 사이클 설정
 */
float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt);

/** @} */ // end of PID_FUNCTIONS

#ifdef __cplusplus
}
#endif

#endif /* __PID_H */
