/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#ifndef HOST_TEST_MODE
#include "stm32g4xx_hal.h"
#else
#include "mock_hal.h"
// Keep Pin definitions if mock_hal.h doesn't have them (it doesn't)
// But wait, pins are usually defined in main.h below.
// The FDIR tests relied on GPIO_PIN_10 being defined here or in the manual block.
// The manual block had #define GPIO_PIN_10 1024.
// Let's keep the Pin definitions if they are not in mock_hal.h
// Actually, mock_hal.h only has types.
// So I should keep the defines or move them to mock_hal.h?
// Moving them to mock_hal.h is cleaner if they are generic HAL things.
// But GPIO_PIN_10 is specific.
// Wait, GPIO_PIN_10 is a standard HAL define.
// I should add standard HAL defines to mock_hal.h or keep them here.
// Let's add them to mock_hal.h to be reusable.
// But for now, I will just keep them here to minimize changes?
// No, the manual block is ugly.
// I'll assume mock_hal.h should provide standard HAL constants.
// I will check mock_hal.h again. It has HAL_OK etc.
// It doesn't have GPIO_PIN_RESET etc.
// I should update mock_hal.h to include GPIO constants.
#include "mock_hal.h"
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "pid.h"
#include "kalman.h"
#include "xcp.h"
#include "telemetry.h"
#include "sensors.h"
#include "actuators.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BAT_measure_Pin GPIO_PIN_1
#define BAT_measure_GPIO_Port GPIOA
#define MCP_RST_Pin GPIO_PIN_4
#define MCP_RST_GPIO_Port GPIOA
#define MS_RST_Pin GPIO_PIN_5
#define MS_RST_GPIO_Port GPIOA
#define Kapton_PWM_Pin GPIO_PIN_6
#define Kapton_PWM_GPIO_Port GPIOA
#define XA1110_Wake_Pin GPIO_PIN_7
#define XA1110_Wake_GPIO_Port GPIOA
#define CM1107N_RST_Pin GPIO_PIN_0
#define CM1107N_RST_GPIO_Port GPIOB
#define SEN_RST_Pin GPIO_PIN_1
#define SEN_RST_GPIO_Port GPIOB
#define GDK_RST_Pin GPIO_PIN_2
#define GDK_RST_GPIO_Port GPIOB
#define PMS_SET_Pin GPIO_PIN_10
#define PMS_SET_GPIO_Port GPIOB
#define SHT_RST_Pin GPIO_PIN_11
#define SHT_RST_GPIO_Port GPIOB
#define XA1110_INT_Pin GPIO_PIN_12
#define XA1110_INT_GPIO_Port GPIOB
#define XA1110_INT_EXTI_IRQn EXTI15_10_IRQn
#define LSM_RST_Pin GPIO_PIN_13
#define LSM_RST_GPIO_Port GPIOB
#define MLX_RST_Pin GPIO_PIN_14
#define MLX_RST_GPIO_Port GPIOB
#define DS18B20_Pin GPIO_PIN_15
#define DS18B20_GPIO_Port GPIOB
#define Minibulb_PWM_Pin GPIO_PIN_6
#define Minibulb_PWM_GPIO_Port GPIOC
#define XA1110_RST_Pin GPIO_PIN_9
#define XA1110_RST_GPIO_Port GPIOA
#define XA1110_PPS_Pin GPIO_PIN_4
#define XA1110_PPS_GPIO_Port GPIOB
#define XA1110_PPS_EXTI_IRQn EXTI4_IRQn
#define LSM_INT_Pin GPIO_PIN_6
#define LSM_INT_GPIO_Port GPIOB
#define LSM_INT_EXTI_IRQn EXTI9_5_IRQn
#define MLX_INT_Pin GPIO_PIN_7
#define MLX_INT_GPIO_Port GPIOB
#define MLX_INT_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
