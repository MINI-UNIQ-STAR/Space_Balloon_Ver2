#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

// --- HAL Typedefs ---
typedef enum {
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define GPIO_PIN_0    0x0001
#define GPIO_PIN_1    0x0002
#define GPIO_PIN_2    0x0004
#define GPIO_PIN_3    0x0008
#define GPIO_PIN_4    0x0010
#define GPIO_PIN_5    0x0020
#define GPIO_PIN_6    0x0040
#define GPIO_PIN_7    0x0080
#define GPIO_PIN_8    0x0100
#define GPIO_PIN_9    0x0200
#define GPIO_PIN_10   0x0400
#define GPIO_PIN_11   0x0800
#define GPIO_PIN_12   0x1000
#define GPIO_PIN_13   0x2000
#define GPIO_PIN_14   0x4000
#define GPIO_PIN_15   0x8000

#define GPIO_MODE_INPUT             0x00000000U
#define GPIO_MODE_OUTPUT_PP         0x00000001U
#define GPIO_MODE_OUTPUT_OD         0x00000011U
#define GPIO_NOPULL                 0x00000000U
#define GPIO_PULLUP                 0x00000001U
#define GPIO_SPEED_FREQ_HIGH        0x00000002U

#define GPIOA ((void*)0x1000)
#define GPIOB ((void*)0x2000)
#define GPIOC ((void*)0x3000)

typedef struct {
  uint32_t Pin;
  uint32_t Mode;
  uint32_t Pull;
  uint32_t Speed;
  uint32_t Alternate;
} GPIO_InitTypeDef;

void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, int PinState);
int HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_Init(void* GPIOx, GPIO_InitTypeDef *GPIO_Init);

extern uint32_t SystemCoreClock;

typedef struct {
    uint32_t Instance; // Mock address
} I2C_HandleTypeDef;

typedef struct {
    uint32_t Instance; // Mock address
} UART_HandleTypeDef;

typedef struct {
    uint32_t Instance;
} ADC_HandleTypeDef;

// --- Helper Macros ---
#define I2C_MEMADD_SIZE_8BIT 0x01
#define I2C_MEMADD_SIZE_16BIT 0x02

// --- Mock State Access ---
// Retrieve last write info to verify driver behavior
typedef struct {
    uint16_t addr;
    uint16_t reg;
    uint8_t data[256];
    uint16_t len;
} MockI2C_LastWrite_t;

MockI2C_LastWrite_t* MockI2C_GetLastWrite(void);
void MockI2C_ClearStats(void);
void MockI2C_SetNextReadData(const uint8_t* data, uint16_t len);

// --- HAL Function Prototypes ---
// Time
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

// I2C
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

// UART
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

// ADC (Minimal)
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc);

// --- Test Helpers ---
void MockHAL_AdvanceTick(uint32_t ms);

#endif // MOCK_HAL_H
