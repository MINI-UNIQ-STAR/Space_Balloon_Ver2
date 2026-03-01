/**
 * @file mock_hal.h
 * @brief HAL Mock for Host Testing (SITL Mode)
 * @details Provides minimal HAL type definitions for PC-based testing
 *          When HOST_TEST_MODE is defined, uses external mock functions
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* HAL Status */
typedef enum {
  HAL_OK = 0x00U,
  HAL_ERROR = 0x01U,
  HAL_BUSY = 0x02U,
  HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

/* GCC/Linux weak attribute support */
#ifndef __weak
#define __weak __attribute__((weak))
#endif

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

extern uint32_t SystemCoreClock;

/* GPIO Pin State */
typedef enum { GPIO_PIN_RESET = 0U, GPIO_PIN_SET = 1U } GPIO_PinState;

/* GPIO Initialization Structure */
typedef struct {
  uint32_t Pin;       /**< 핀 번호 */
  uint32_t Mode;      /**< 모드 */
  uint32_t Pull;      /**< 풀업/풀다운 */
  uint32_t Speed;     /**< 속도 */
  uint32_t Alternate; /**< Alt Function */
} GPIO_InitTypeDef;

#define GPIO_MODE_INPUT 0x00000000U
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_AF_PP 0x00000002U
#define GPIO_NOPULL 0x00000000U
#define GPIO_PULLUP 0x00000001U
#define GPIO_SPEED_FREQ_LOW 0x00000000U
#define GPIO_SPEED_FREQ_HIGH 0x00000002U

/* GPIO Pin Definitions */
#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_1 ((uint16_t)0x0002)
#define GPIO_PIN_2 ((uint16_t)0x0004)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)
#define GPIO_PIN_7 ((uint16_t)0x0080)
#define GPIO_PIN_8 ((uint16_t)0x0100)
#define GPIO_PIN_9 ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)
#define GPIO_PIN_All ((uint16_t)0xFFFF)

/* GPIO Port Mock */
typedef struct {
  uint32_t dummy;
} GPIO_TypeDef;

/* I2C Handle Mock */
typedef struct {
  void *Instance;
  uint32_t State;
} I2C_HandleTypeDef;

/* UART Handle Mock */
typedef struct {
  void *Instance;
  uint32_t gState;
  void *hdmarx;
  void *hdmatx;
} UART_HandleTypeDef;

#define HAL_UART_STATE_READY 0x20U
#define HAL_UART_STATE_BUSY_RX 0x22U

/* ADC Handle Mock */
typedef struct {
  void *Instance;
} ADC_HandleTypeDef;

/* TIM Handle Mock */
typedef struct {
  void *Instance;
} TIM_HandleTypeDef;

/* Mock GPIO Port Pointers */
#define GPIOA ((GPIO_TypeDef *)0x48000000U)
#define GPIOB ((GPIO_TypeDef *)0x48000400U)
#define GPIOC ((GPIO_TypeDef *)0x48000800U)

/* Mock USART Pointers */
#define USART1 ((void *)1)
#define USART2 ((void *)2)
#define USART3 ((void *)3)

/* Mock I2C Pointers */
#define I2C1 ((void *)1)
#define I2C3 ((void *)3)

/* I2C Memory Address Sizes */
#define I2C_MEMADD_SIZE_8BIT 0x00000001U
#define I2C_MEMADD_SIZE_16BIT 0x00000010U

/* ========================================================================== */
/* Mock Functions                                                           */
/* ========================================================================== */

#ifdef HOST_TEST_MODE
/* Common stubs */
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t ms);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *GPIO_Init);

/* I2C stubs */
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c,
                                    uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size,
                                   uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c,
                                          uint16_t DevAddress, uint8_t *pData,
                                          uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c,
                                         uint16_t DevAddress, uint8_t *pData,
                                         uint16_t Size, uint32_t Timeout);

/* UART stubs */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData,
                                   uint16_t Size, uint32_t Timeout);

/* ADC stubs */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc,
                                            uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);

/* TIM/PWM stubs */
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
void __HAL_TIM_SET_COMPARE(TIM_HandleTypeDef *htim, uint32_t Channel,
                           uint32_t Compare);

/* ========================================================================== */
/* Mock State Access (Unit Testing)                                         */
/* ========================================================================== */

/** @brief 마지막 I2C 쓰기 작업 기록 */
typedef struct {
  uint16_t addr;     /**< 디바이스 주소 */
  uint16_t reg;      /**< 레지스터 주소 */
  uint8_t data[256]; /**< 기록된 데이터 */
  uint16_t len;      /**< 데이터 길이 */
} MockI2C_LastWrite_t;

MockI2C_LastWrite_t *MockI2C_GetLastWrite(void);
void MockI2C_ClearStats(void);
void MockI2C_SetNextReadData(const uint8_t *data, uint16_t len);
void MockHAL_AdvanceTick(uint32_t ms);

/** @brief 마지막 UART 송신 작업 기록 */
typedef struct {
  uint8_t data[256]; /**< 송신된 데이터 */
  uint16_t len;      /**< 데이터 길이 */
} MockUART_LastTx_t;

void MockUART_ClearStats(void);
MockUART_LastTx_t *MockUART_GetLastTx(void);

#else
/* Inline stubs for non-test builds */
static inline uint32_t HAL_GetTick(void) { return 0; }
static inline void HAL_Delay(uint32_t ms) { (void)ms; }
#endif /* HOST_TEST_MODE */

#define UNUSED(x) ((void)(x))

#endif /* MOCK_HAL_H */
