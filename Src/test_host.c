#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "app.h"
#include "telemetry.h"

// Mock HAL Handle Definitions (Since we don't have stm32g4xx_hal.h)
// These must match what external modules expect (pointers)
typedef struct { uint32_t Instance; } I2C_HandleTypeDef;
typedef struct { uint32_t Instance; } UART_HandleTypeDef;
typedef struct { uint32_t Instance; } TIM_HandleTypeDef;
typedef struct { uint32_t Instance; } ADC_HandleTypeDef;

// Mock Handles
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim16;
ADC_HandleTypeDef hadc1;

// Mock HAL Functions
void HAL_Delay(uint32_t ms) {}
uint32_t HAL_GetTick(void) { static uint32_t tick = 0; tick+=20; return tick; }

int main(void) {
    printf("Starting Host Test (App Layer)...\n");
    
    // 1. App Init
    App_Init();
    printf("[Pass] App Initialized.\n");
    
    // 2. Loop
    printf("Running Loop...\n");
    for(int i=0; i<50; i++) {
        App_Loop();
        
        // Print Status every 10 ticks
        if (i%10 == 0) {
            printf("Seq: %d, Fix: %d, Bat: %d mV, Roll: %.1f, Pitch: %.1f\n", 
                   telem_frame.seq, 
                   telem_frame.payload.gps_fix,
                   telem_frame.payload.bat_mv,
                   telem_frame.payload.kf_roll_deg,
                   telem_frame.payload.kf_pitch_deg);
        }
    }
    printf("Test Finished.\n");
    return 0;
}
