/**
 * @file actuators.h
 * @brief 액추에이터 제어 인터페이스 - 히터 PWM 제어
 * @details 2개 히터 PWM 출력 제어
 *          배터리 히터 (Kapton): PA6, TIM3 CH1, 7.2W @ 5V, Max 60% 듀티
 *          보드 히터 (Minibulb): PC6, TIM8 CH1, ~4W @ 5V, Max 100% 듀티
 *          PWM 주파수: 1kHz (1ms 주기)
 *          분해능: 10비트 (0-1023 카운트)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __ACTUATORS_H
#define __ACTUATORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @defgroup ACTUATORS_PINS 히터 PWM 핀 정의
 * @{
 */

/** @brief 배터리 히터 PWM 핀 (PA6, TIM3 CH1, AF2) */
#define HEATER_BATTERY_PIN      GPIO_PIN_6
#define HEATER_BATTERY_PORT     GPIOA

/** @brief 보드 히터 PWM 핀 (PC6, TIM8 CH1, AF4) */
#define HEATER_BOARD_PIN        GPIO_PIN_6
#define HEATER_BOARD_PORT       GPIOC

/** @} */ // end of ACTUATORS_PINS

/**
 * @defgroup ACTUATORS_FUNCTIONS 액추에이터 제어 함수
 * @{
 */

/**
 * @brief 액추에이터 초기화
 * @details 초기화 순서:
 *          1. PWM 타이머 시작 (TIM3, TIM8)
 *          2. 초기 듀티 사이클 0% 설정 (히터 OFF)
 *          3. GPIO Alternate Function 설정 (HAL에서 이미 수행)
 *          PWM 설정:
 *          - 주파수: 1kHz (ARR=1023, PSC=170-1, F_CLK=170MHz)
 *          - 듀티 사이클: CCRx 레지스터로 제어 (0-1023)
 * @note App_Init()에서 1회 호출
 *       HAL_TIM_PWM_Init() 후 호출 필요
 */
void Actuators_Init(void);

/**
 * @brief 배터리 히터 듀티 사이클 설정
 * @param[in] duty_percent 듀티 사이클 (0.0 ~ 100.0%)
 * @details 하드웨어:
 *          - Kapton 히터: 7.2W @ 5V = 1.44A (최대 전류)
 *          - TIM3 CH1 (PA6)
 *          - 전력 예산: 최대 60% 듀티 (평균 0.86A)
 *          제어 로직:
 *          - duty_percent > 60.0 → 60.0으로 클램핑 (app.c에서 수행)
 *          - CCR1 = (duty_percent / 100.0) × 1023
 * @note App_Loop()에서 PID 제어기 출력값으로 호출 (50Hz)
 *       저전압 모드 시 자동 0% 설정
 */
void Actuators_SetHeater_Battery(float duty_percent);

/**
 * @brief 보드 히터 듀티 사이클 설정
 * @param[in] duty_percent 듀티 사이클 (0.0 ~ 100.0%)
 * @details 하드웨어:
 *          - Minibulb 히터: ~4W @ 5V = 0.8A (최대 전류)
 *          - TIM8 CH1 (PC6)
 *          - 전력 예산: 최대 100% 듀티 (하드웨어 테스트 필요)
 *          제어 로직:
 *          - CCR1 = (duty_percent / 100.0) × 1023
 * @note App_Loop()에서 PID 제어기 출력값으로 호출 (50Hz)
 *       저전압 모드 시 자동 0% 설정
 */
void Actuators_SetHeater_Board(float duty_percent);

/** @} */ // end of ACTUATORS_FUNCTIONS

#ifdef __cplusplus
}
#endif

#endif /* __ACTUATORS_H */
