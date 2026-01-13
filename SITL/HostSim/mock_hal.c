/**
 * @file mock_hal.c
 * @brief STM32 HAL 레이어 Mock 구현 (SITL용)
 * @details 호스트 시뮬레이션을 위해 하드웨어 의존적인 HAL 함수들을 가상화
 *          - 시스템 틱 (HAL_GetTick) 시뮬레이션
 *          - UART 출력을 콘솔(stdout)로 리다이렉션
 *          - I2C, ADC, GPIO 등 주변장치 함수 스텁(Stub) 처리
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "mock_hal.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief 에러 핸들러 (Mock)
 * @details 실제 하드웨어 리셋 대신 에러 메시지 출력 후 계속 진행
 */
void Error_Handler(void) { printf("Error_Handler called\n"); }

/** @brief 시스템 클럭 주파수 (170MHz 시뮬레이션) */
uint32_t SystemCoreClock = 170000000; 

// HAL Tick
static uint32_t mock_tick = 0;

/**
 * @brief 현재 시스템 틱 조회
 * @return uint32_t 부팅 후 경과 시간 (ms)
 */
uint32_t HAL_GetTick(void) { return mock_tick; }

/**
 * @brief 시간 지연 (Blocking)
 * @param ms 지연 시간 (ms)
 * @details 시뮬레이션 시간을 강제로 증가시킴
 */
void HAL_Delay(uint32_t ms) { mock_tick += ms; }

/**
 * @brief 시뮬레이션 시간 전진 (Test용)
 * @param ms 증가시킬 시간 (ms)
 */
void MockHAL_AdvanceTick(uint32_t ms) { mock_tick += ms; }

// I2C Stubs
/** @brief I2C 메모리 읽기 스텁 */
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    memset(pData, 0, Size);
    return HAL_OK;
}
/** @brief I2C 메모리 쓰기 스텁 */
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_OK;
}
/** @brief I2C 마스터 전송 스텁 */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_OK;
}
/** @brief I2C 마스터 수신 스텁 */
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    memset(pData, 0, Size);
    return HAL_OK;
}

// GPIO Stubs
/** @brief GPIO 핀 쓰기 스텁 */
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {}
/** @brief GPIO 핀 읽기 스텁 (항상 HIGH 리턴) */
GPIO_PinState HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin) { return GPIO_PIN_SET; }
/** @brief GPIO 초기화 스텁 */
void HAL_GPIO_Init(void* GPIOx, GPIO_InitTypeDef *GPIO_Init) {}

// UART Stubs
/** 
 * @brief UART 전송 스텁
 * @details UART 출력을 stdout(콘솔)으로 리다이렉션하여 디버깅 지원
 */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    // Print to console for debug
    for(int i=0; i<Size && pData[i]!=0; i++) putchar(pData[i]);
    return HAL_OK;
}
/** @brief UART 수신 스텁 (타임아웃 반환) */
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_TIMEOUT;
}

// ADC Stubs
/** @brief ADC 시작 스텁 */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc) { return HAL_OK; }
/** @brief ADC 변환 대기 스텁 */
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout) { return HAL_OK; }
/** @brief ADC 값 읽기 스텁 (고정값 2048 리턴) */
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc) { return 2048; }
/** @brief ADC 정지 스텁 */
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc) { return HAL_OK; }

// Note: Actuator stubs removed - use Core/Src/actuators.c or add them conditionally
