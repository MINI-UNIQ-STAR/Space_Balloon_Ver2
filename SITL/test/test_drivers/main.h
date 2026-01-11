#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "mock_hal.h"

// Mock Error Handler
void Error_Handler(void);

// Mock GPIO Ports (Pointers)
#define GPIOB ((void*)0x2)
#define GPIOC ((void*)0x3)

// Mock Pin Labels
#define PMS_SET_GPIO_Port GPIOB
#define PMS_SET_Pin GPIO_PIN_0

#define XA1110_RST_GPIO_Port GPIOC
#define XA1110_RST_Pin GPIO_PIN_1
#define XA1110_Wake_GPIO_Port GPIOC
#define XA1110_Wake_Pin GPIO_PIN_2
#define MCP_RST_GPIO_Port GPIOC
#define MCP_RST_Pin GPIO_PIN_3
#define MS_RST_GPIO_Port GPIOC
#define MS_RST_Pin GPIO_PIN_4
#define CM1107N_RST_GPIO_Port GPIOC
#define CM1107N_RST_Pin GPIO_PIN_5
#define SEN_RST_GPIO_Port GPIOC
#define SEN_RST_Pin GPIO_PIN_6
#define MLX_RST_GPIO_Port GPIOC
#define MLX_RST_Pin GPIO_PIN_7

#include "pid.h"
#include "kalman.h"
#include "xcp.h"
#include "telemetry.h"
#include "sensors.h"
#include "actuators.h"

#define DEBUG_PRINT printf

#endif
