/**
 * @file mock_hal.h
 * @brief HAL Mock for Host Testing (SITL Mode)
 * @details Provides minimal HAL type definitions for PC-based testing
 *          When HOST_TEST_MODE is defined, uses external mock functions
 */

#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>

/* HAL Status */
typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/* GPIO Pin State */
typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET   = 1U
} GPIO_PinState;

/* GPIO Pin Definitions */
#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_2   ((uint16_t)0x0004)
#define GPIO_PIN_3   ((uint16_t)0x0008)
#define GPIO_PIN_4   ((uint16_t)0x0010)
#define GPIO_PIN_5   ((uint16_t)0x0020)
#define GPIO_PIN_6   ((uint16_t)0x0040)
#define GPIO_PIN_7   ((uint16_t)0x0080)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_11  ((uint16_t)0x0800)
#define GPIO_PIN_12  ((uint16_t)0x1000)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)
#define GPIO_PIN_15  ((uint16_t)0x8000)

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

/* UART State Definitions */
#define HAL_UART_STATE_READY    0x20U
#define HAL_UART_STATE_BUSY_RX  0x22U

/* ADC Handle Mock */
typedef struct {
    void *Instance;
} ADC_HandleTypeDef;

/* TIM Handle Mock */
typedef struct {
    void *Instance;
} TIM_HandleTypeDef;

/* Mock GPIO Port Pointers - Use unique addresses for comparison */
#define GPIOA ((GPIO_TypeDef*)0x48000000U)
#define GPIOB ((GPIO_TypeDef*)0x48000400U)
#define GPIOC ((GPIO_TypeDef*)0x48000800U)

/* Mock USART Pointers */
#define USART1 ((void*)1)
#define USART2 ((void*)2)
#define USART3 ((void*)3)

/* Mock I2C Pointers */
#define I2C1 ((void*)1)
#define I2C3 ((void*)3)

/* ========================================================================== */
/* Mock Functions - External declarations for HOST_TEST_MODE                */
/* ========================================================================== */

#ifdef HOST_TEST_MODE
/* External mock functions - defined in mock_dependencies.c */
extern uint32_t HAL_GetTick(void);
extern void HAL_Delay(uint32_t ms);
extern void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
extern GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
extern void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);
#else
/* Inline stubs for non-test builds */
static inline uint32_t HAL_GetTick(void) {
    static uint32_t tick = 0;
    return tick++;
}

static inline void HAL_Delay(uint32_t ms) {
    (void)ms;
}

static inline void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state) {
    (void)port; (void)pin; (void)state;
}

static inline GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin) {
    (void)port; (void)pin;
    return GPIO_PIN_RESET;
}

static inline void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin) {
    (void)port; (void)pin;
}
#endif /* HOST_TEST_MODE */

#endif /* MOCK_HAL_H */
