#include "main.h"
#include <stdio.h>

void Error_Handler(void) { printf("Error_Handler called\n"); }

// Stubs for HAL I2C/GPIO used in sensors.c
int HAL_I2C_Mem_Read(void* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0; // HAL_OK
}
int HAL_I2C_Mem_Write(void* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0;
}
int HAL_I2C_Master_Transmit(void* hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0;
}
int HAL_I2C_Master_Receive(void* hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0;
}
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, int PinState) {}
int HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin) { return 0; }
void HAL_GPIO_Init(void* GPIOx, void* GPIO_Init) {}

// UART
int HAL_UART_Transmit(void* huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    // Print to console?
    for(int i=0; i<Size; i++) putchar(pData[i]);
    return 0;
}

// System
// HAL_Delay and HAL_GetTick are in test_host.c

// Actuator Mocks
void Actuators_Init(void) {}
void Actuators_SetHeater_Battery(float duty) {}
void Actuators_SetHeater_Board(float duty) {}
