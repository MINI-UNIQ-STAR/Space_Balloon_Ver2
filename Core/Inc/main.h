/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : 메인 헤더 파일 - 애플리케이션 공통 정의
 * @details        : STM32G431CBU6 HAL 드라이버 인클루드 및 GPIO 핀 정의
 *                   STM32CubeMX 자동 생성 코드 + 사용자 정의 인클루드
 * @author         : SpaceBalloon Team
 * @date           : 2026-01-13
 * @version        : 1.0
 ******************************************************************************
 */

/* USER CODE BEGIN Header */
#if defined(HOST_TEST_MODE)
// SITL 테스트 모드: 표준 HAL 인클루드 억제 (mock_hal.h 사용)
#define STM32G4xx_HAL_H
#endif
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#ifndef STM32G4xx_HAL_H
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_i2c.h"
#include "stm32g4xx_hal_uart.h"
#include "stm32g4xx_hal_gpio.h"
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#if defined(HOST_TEST_MODE)
#include "mock_hal.h"  /**< SITL 모드용 HAL Mock (호스트 테스트) */
#endif

#include "pid.h"        /**< PID 제어기 인터페이스 */
#include "kalman.h"     /**< 칼만 필터 인터페이스 */
#include "xcp.h"        /**< XCP 캘리브레이션 프로토콜 */
#include "telemetry.h"  /**< 텔레메트리 프레임 정의 */
#include "sensors.h"    /**< 센서 드라이버 인터페이스 */
#include "actuators.h"  /**< 액추에이터 제어 인터페이스 */
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
/**
 * @brief 에러 핸들러 - 시스템 크리티컬 에러 처리
 * @details 동작:
 *          1. 모든 인터럽트 비활성화
 *          2. 무한 루프 진입 (디버거 브레이크포인트 위치)
 *          호출 조건:
 *          - HAL 초기화 실패 (I2C, UART, ADC 등)
 *          - 하드웨어 오류 (클럭 설정 실패)
 * @note 디버그 모드에서 이 함수에 브레이크포인트 설정 권장
 */
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/**
 * @defgroup GPIO_PINS GPIO 핀 정의 (STM32CubeMX 자동 생성)
 * @{
 */

/* Private defines -----------------------------------------------------------*/

/** @brief 배터리 전압 측정 ADC 핀 (PA1, ADC1_IN2) */
#define BAT_measure_Pin GPIO_PIN_1
#define BAT_measure_GPIO_Port GPIOA

/** @brief MCP9600 리셋 핀 (PA4, Active Low) */
#define MCP_RST_Pin GPIO_PIN_4
#define MCP_RST_GPIO_Port GPIOA

/** @brief MS5611 리셋 핀 (PA5, Active Low) */
#define MS_RST_Pin GPIO_PIN_5
#define MS_RST_GPIO_Port GPIOA

/** @brief Kapton 배터리 히터 PWM 핀 (PA6, TIM3 CH1) */
#define Kapton_PWM_Pin GPIO_PIN_6
#define Kapton_PWM_GPIO_Port GPIOA

/** @brief XA1110 GPS Wake 핀 (PA7, Active High) */
#define XA1110_Wake_Pin GPIO_PIN_7
#define XA1110_Wake_GPIO_Port GPIOA

/** @brief CM1107N CO2 센서 리셋 핀 (PB0, Active Low) */
#define CM1107N_RST_Pin GPIO_PIN_0
#define CM1107N_RST_GPIO_Port GPIOB

/** @brief SEN0321 오존 센서 리셋 핀 (PB1, Active Low) */
#define SEN_RST_Pin GPIO_PIN_1
#define SEN_RST_GPIO_Port GPIOB

/** @brief GDK101 방사선 센서 리셋 핀 (PB2, Active Low) */
#define GDK_RST_Pin GPIO_PIN_2
#define GDK_RST_GPIO_Port GPIOB

/** @brief PMS3003 미세먼지 센서 SET 핀 (PB10, Active High) */
#define PMS_SET_Pin GPIO_PIN_10
#define PMS_SET_GPIO_Port GPIOB

/** @brief SHT31 온습도 센서 리셋 핀 (PB11, Active Low) */
#define SHT_RST_Pin GPIO_PIN_11
#define SHT_RST_GPIO_Port GPIOB

/** @brief XA1110 GPS 인터럽트 핀 (PB12, EXTI12, Rising Edge) */
#define XA1110_INT_Pin GPIO_PIN_12
#define XA1110_INT_GPIO_Port GPIOB
#define XA1110_INT_EXTI_IRQn EXTI15_10_IRQn

/** @brief LSM6DSV16X IMU 리셋 핀 (PB13, Active Low) */
#define LSM_RST_Pin GPIO_PIN_13
#define LSM_RST_GPIO_Port GPIOB

/** @brief MLX90393 자기계 리셋 핀 (PB14, Active Low) */
#define MLX_RST_Pin GPIO_PIN_14
#define MLX_RST_GPIO_Port GPIOB

/** @brief DS18B20 1-Wire 온도 센서 핀 (PB15, Open-Drain) */
#define DS18B20_Pin GPIO_PIN_15
#define DS18B20_GPIO_Port GPIOB

/** @brief Minibulb 보드 히터 PWM 핀 (PC6, TIM8 CH1) */
#define Minibulb_PWM_Pin GPIO_PIN_6
#define Minibulb_PWM_GPIO_Port GPIOC

/** @brief XA1110 GPS 리셋 핀 (PA9, Active Low) */
#define XA1110_RST_Pin GPIO_PIN_9
#define XA1110_RST_GPIO_Port GPIOA

/** @brief XA1110 GPS 1PPS 신호 핀 (PB4, EXTI4, Rising Edge) */
#define XA1110_PPS_Pin GPIO_PIN_4
#define XA1110_PPS_GPIO_Port GPIOB
#define XA1110_PPS_EXTI_IRQn EXTI4_IRQn

/** @brief LSM6DSV16X IMU 인터럽트 핀 (PB6, EXTI6, Rising Edge) */
#define LSM_INT_Pin GPIO_PIN_6
#define LSM_INT_GPIO_Port GPIOB
#define LSM_INT_EXTI_IRQn EXTI9_5_IRQn

/** @brief MLX90393 자기계 인터럽트 핀 (PB7, EXTI7, Rising Edge) */
#define MLX_INT_Pin GPIO_PIN_7
#define MLX_INT_GPIO_Port GPIOB
#define MLX_INT_EXTI_IRQn EXTI9_5_IRQn

/** @} */ // end of GPIO_PINS

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
