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

void Sensors_Reset(SensorID_t id) {
    sensors_reset_call_count++;
    last_reset_sensor = id;
}

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
