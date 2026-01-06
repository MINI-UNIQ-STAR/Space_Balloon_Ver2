#include "main.h"
#include "sensors.h"
#include "unity.h"

// --- Mock HAL State ---
static uint32_t mock_tick_ms = 0;

void MockHAL_SetTick(uint32_t tick) {
    mock_tick_ms = tick;
}

void MockHAL_AdvanceTick(uint32_t delta) {
    mock_tick_ms += delta;
}

uint32_t HAL_GetTick(void) {
    return mock_tick_ms;
}

// --- Mock GPIO State ---
static int pms_set_state = -1;

#include <stdio.h>

void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    printf("DEBUG: MockGPIO Write: Port=%p, Pin=%u, State=%d (Expected Port=%p, Pin=%u)\n", 
           GPIOx, GPIO_Pin, PinState, PMS_SET_GPIO_Port, PMS_SET_Pin);
    
    if (GPIOx == PMS_SET_GPIO_Port && GPIO_Pin == PMS_SET_Pin) {
        pms_set_state = PinState;
    }
}

int MockGPIO_GetPMSSetState(void) {
    return pms_set_state;
}

// --- Mock Sensors ---
static int sensors_reset_call_count = 0;
static SensorID_t last_reset_sensor = -1;

#ifndef INTEGRATION_TEST
void Sensors_Reset(SensorID_t id) {
    sensors_reset_call_count++;
    last_reset_sensor = id;
}
#endif

void MockSensors_ClearStats(void) {
    sensors_reset_call_count = 0;
    last_reset_sensor = -1;
    pms_set_state = -1;
    mock_tick_ms = 0;
}

int MockSensors_GetResetCount(void) {
    return sensors_reset_call_count;
}

int MockSensors_GetLastResetSensor(void) {
    return (int)last_reset_sensor;
}

// --- Mock HAL I2C ---
int HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0; // HAL_OK
}

int HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0; // HAL_OK
}

int HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0; // HAL_OK
}

int HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return 0; // HAL_OK
}
