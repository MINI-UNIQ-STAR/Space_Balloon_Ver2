#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdbool.h>

// Mock HAL GPIO Definitions
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET   1
typedef uint8_t GPIO_PinState;

// Pin Definitions matched to Core/Inc/main.h used by fdir.c
#define PMS_SET_Pin 1024
#define PMS_SET_GPIO_Port (void*)0x4000

// Mock HAL Functions
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, int PinState);

#endif
