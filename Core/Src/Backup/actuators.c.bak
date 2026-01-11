#include "actuators.h"

#ifndef UNIT_TEST
// Real Hardware Handles
#include "tim.h" // Assuming main.h or tim.h defines these
extern TIM_HandleTypeDef htim3; // PA6 - Heater 1
extern TIM_HandleTypeDef htim8; // PC6 - Heater 2
#endif

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
