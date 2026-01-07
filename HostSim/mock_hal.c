#include "mock_hal.h"
#include <stdio.h>
#include <string.h>

// Error Handler
void Error_Handler(void) { printf("Error_Handler called\n"); }

// HAL Tick
static uint32_t mock_tick = 0;
uint32_t HAL_GetTick(void) { return mock_tick; }
void HAL_Delay(uint32_t ms) { mock_tick += ms; }
void MockHAL_AdvanceTick(uint32_t ms) { mock_tick += ms; }

// I2C Stubs
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    memset(pData, 0, Size);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    memset(pData, 0, Size);
    return HAL_OK;
}

// GPIO Stubs
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {}
GPIO_PinState HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin) { return GPIO_PIN_SET; }
void HAL_GPIO_Init(void* GPIOx, GPIO_InitTypeDef *GPIO_Init) {}

// UART Stubs
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    // Print to console for debug
    for(int i=0; i<Size && pData[i]!=0; i++) putchar(pData[i]);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_TIMEOUT;
}

// ADC Stubs
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc) { return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout) { return HAL_OK; }
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc) { return 2048; }
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc) { return HAL_OK; }

// Note: Actuator stubs removed - use Core/Src/actuators.c or add them conditionally
