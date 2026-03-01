/**
 * @file actuators.c
 * @brief 액추에이터 제어 모듈 구현 - PWM 히터 제어
 * @details 배터리 히터와 보드 히터의 PWM 듀티 사이클 제어
 *          - 배터리 히터: TIM3 CH1 (PA6) - Kapton 히터 7.2W @ 5V
 *          - 보드 히터: TIM8 CH1 (PC6) - Minibulb 히터 ~4W @ 5V
 *          - ARR = 1000으로 가정 (0.1% 해상도)
 *          - 전력 예산 보호: 배터리 히터 60% 제한
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "actuators.h"

#ifndef UNIT_TEST
// Real Hardware Handles
#include "tim.h" // Assuming main.h or tim.h defines these
extern TIM_HandleTypeDef htim3; // PA6 - Heater 1
extern TIM_HandleTypeDef htim8; // PC6 - Heater 2
#endif

/**
 * @brief 액추에이터 초기화 (PWM 히터)
 * @details PWM 타이머 시작
 *          - TIM3 CH1: 배터리 히터
 *          - TIM8 CH1: 보드 히터
 * @note main() 초기화 시퀀스에서 App_Init()을 통해 호출됨
 */
void Actuators_Init(void) {
#ifndef UNIT_TEST
    // Start PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
#else
    // Mock Init
    // printf("Actuators_Init: PWM Mock Started\n");
#endif
}

/**
 * @brief 배터리 히터 PWM 듀티 사이클 설정
 * @param duty_percent 듀티 사이클 (0.0 ~ 100.0%)
 * @details Kapton 히터 제어 (7.2W @ 5V, 1.44A)
 *          - 듀티 사이클 범위: 0.0% ~ 100.0%
 *          - 전력 예산 보호: app.c에서 60% 제한 적용
 *          - CCR 계산: duty_percent * 10 (ARR=1000 가정)
 *          - 타이머: TIM3 CH1 (PA6)
 */
void Actuators_SetHeater_Battery(float duty_percent) {
    if (duty_percent < 0.0f) {
        duty_percent = 0.0f;
    }
    if (duty_percent > 100.0f) {
        duty_percent = 100.0f;
    }
    
    // Calculate CCR value based on Timer Period (ARR)
    // Assuming ARR = 1000 for simple mapping
    uint32_t ccr_val = (uint32_t)(duty_percent * 10.0f); 
    
#ifndef UNIT_TEST
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr_val);
#else
    // Mock Output
    // printf("Heater Bat Set: %.1f%% (CCR %d)\n", duty_percent, ccr_val);
#endif
}

/**
 * @brief 보드 히터 PWM 듀티 사이클 설정
 * @param duty_percent 듀티 사이클 (0.0 ~ 100.0%)
 * @details Minibulb 히터 제어 (~4W @ 5V)
 *          - 듀티 사이클 범위: 0.0% ~ 100.0%
 *          - CCR 계산: duty_percent * 10 (ARR=1000 가정)
 *          - 타이머: TIM8 CH1 (PC6)
 */
void Actuators_SetHeater_Board(float duty_percent) {
    if (duty_percent < 0.0f) {
        duty_percent = 0.0f;
    }
    if (duty_percent > 100.0f) {
        duty_percent = 100.0f;
    }
    
    uint32_t ccr_val = (uint32_t)(duty_percent * 10.0f);
    
#ifndef UNIT_TEST
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, ccr_val);
#else
    // Mock Output
    // printf("Heater Board Set: %.1f%% (CCR %d)\n", duty_percent, ccr_val);
#endif
}
