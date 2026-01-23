/**
 * @file pid.c
 * @brief PID 제어기 구현 - 히터 온도 제어
 * @details 조건부 적분 Anti-Windup PID 제어기
 *          - 배터리 히터: Kp=400, Ki=6, Kd=0, 목표=10°C, 최대출력=60%
 *          - 보드 히터: Kp=500, Ki=5, Kd=0, 목표=5°C, 최대출력=100%
 *          - Anti-Windup: 출력 포화 시 적분 항 누적 중지
 *          - 출력 범위: 0% ~ MaxOutput%
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "bsp.h" /* for float32_t */
#include "pid.h"
#include <stdbool.h>

/**
 * @brief PID 제어기 초기화
 * @param hpid PID 핸들
 * @param Kp 비례 이득
 * @param Ki 적분 이득
 * @param Kd 미분 이득
 * @param MaxOutput 최대 출력 (%)
 * @details 초기 상태:
 *          - 목표값 = 0
 *          - 적분 누적 = 0
 *          - 이전 오차 = 0
 */
void PID_Init(PID_HandleTypeDef *hpid, float32_t Kp, float32_t Ki, float32_t Kd, float32_t MaxOutput) {
    hpid->Kp = Kp;
    hpid->Ki = Ki;
    hpid->Kd = Kd;
    hpid->MaxOutput = MaxOutput;
    
    hpid->Target = 0.0f;
    hpid->IntegratedError = 0.0f;
    hpid->LastError = 0.0f;
}

/**
 * @brief PID 제어기 업데이트
 * @param hpid PID 핸들
 * @param measurement 현재 측정값 (온도, °C)
 * @param dt 샘플 주기 (초, 50Hz = 0.02s)
 * @return float 제어 출력 (0 ~ MaxOutput%)
 * @details 제어 알고리즘:
 *          1. 오차 계산: e = Target - measurement
 *          2. 비례 항: P = Kp * e
 *          3. 미분 항: D = Kd * de/dt
 *          4. 조건부 적분 (Anti-Windup):
 *             - 출력 포화 시 적분 중지
 *             - 포화 조건: (output >= Max && e > 0) || (output <= 0 && e < 0)
 *          5. 적분 항: I = Ki * ∫e dt
 *          6. 출력: u = P + I + D
 *          7. 출력 제한: 0 ~ MaxOutput
 */
float32_t PID_Update(PID_HandleTypeDef *hpid, float32_t measurement, float32_t dt) {
    float32_t error = hpid->Target - measurement;
    
    /* Proportional term */
    float32_t p_term = hpid->Kp * error;
    
    /* Derivative term */
    float32_t derivative = (error - hpid->LastError) / dt;
    float32_t d_term = hpid->Kd * derivative;
    hpid->LastError = error;
    
    /* Tentative output without new I-term contribution */
    float32_t tentative_output = p_term + (hpid->Ki * hpid->IntegratedError) + d_term;
    
    /* Conditional Integration (Anti-windup):
     * Only accumulate I-term if output is not saturated.
     * Saturated means: output >= MaxOutput OR output <= 0 when error pushes further */
    bool saturated_high = (tentative_output >= hpid->MaxOutput) && (error > 0.0f);
    bool saturated_low = (tentative_output <= 0.0f) && (error < 0.0f);
    
    if (!saturated_high && !saturated_low) {
        hpid->IntegratedError += error * dt;
    }
    
    float32_t i_term = hpid->Ki * hpid->IntegratedError;
    float32_t output = p_term + i_term + d_term;
    
    /* Output Clamping */
    if (output > hpid->MaxOutput) {
        output = hpid->MaxOutput;
    } else if (output < 0.0f) {
        output = 0.0f;
    }
    
    return output;
}
