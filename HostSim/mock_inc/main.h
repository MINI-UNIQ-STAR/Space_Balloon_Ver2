#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// Mock HAL Status
typedef enum 
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

// Mock Error Handler
void Error_Handler(void);

// Mock GPIO definitions (if needed by sensors.c or app.c)
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
// typedef uint32_t uint16_t; // Removed: defined in stdint.h
// typedef uint32_t uint8_t; // Removed: defined in stdint.h

// Add any other macros used in your App logic here
#define DEBUG_PRINT printf

#endif
