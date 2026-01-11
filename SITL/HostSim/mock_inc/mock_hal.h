#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// HAL Status
typedef enum 
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

// CMSIS Mocks
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

extern uint32_t SystemCoreClock;

// GPIO State
typedef enum
{
  GPIO_PIN_RESET = 0u,
  GPIO_PIN_SET
} GPIO_PinState;

typedef struct
{
  uint32_t Pin;
  uint32_t Mode;
  uint32_t Pull;
  uint32_t Speed;
  uint32_t Alternate;
} GPIO_InitTypeDef;

// GPIO Modes
#define GPIO_MODE_INPUT     0x00000000U
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_OUTPUT_OD 0x00000011U
#define GPIO_NOPULL         0x00000000U
#define GPIO_PULLUP         0x00000001U
#define GPIO_SPEED_FREQ_LOW  0x00000000U
#define GPIO_SPEED_FREQ_HIGH 0x00000002U

// GPIO Mock
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin);

// Time Mock
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

// Initialization
HAL_StatusTypeDef HAL_Init(void);

// --- Handle Types (Mock) ---
typedef struct {
    uint32_t Instance;
} I2C_HandleTypeDef;

typedef struct {
    uint32_t Instance;
} UART_HandleTypeDef;

typedef struct {
    uint32_t Instance;
} ADC_HandleTypeDef;

typedef struct {
    uint32_t Instance;
} TIM_HandleTypeDef;

// --- Helper Macros ---
#define I2C_MEMADD_SIZE_8BIT 0x01
#define I2C_MEMADD_SIZE_16BIT 0x02

// --- I2C Prototypes ---
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

// --- UART Prototypes ---
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

// --- ADC Prototypes ---
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc);

// --- Timer/PWM Prototypes ---
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
void __HAL_TIM_SET_COMPARE(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t Compare);

// --- Mock State Access (For Unit Tests) ---
typedef struct {
    uint16_t addr;
    uint16_t reg;
    uint8_t data[256];
    uint16_t len;
} MockI2C_LastWrite_t;

MockI2C_LastWrite_t* MockI2C_GetLastWrite(void);
void MockI2C_ClearStats(void);
void MockI2C_SetNextReadData(const uint8_t* data, uint16_t len);
void MockHAL_AdvanceTick(uint32_t ms);

// --- Mock UART State ---
typedef struct {
    uint8_t data[256];
    uint16_t len;
} MockUART_LastTx_t;

void MockUART_ClearStats(void);
MockUART_LastTx_t* MockUART_GetLastTx(void);

// Macro Mocks
#define UNUSED(x) ((void)(x))

// Add other HAL types/macros as needed by the application
// For example, if app.c uses specific GPIO ports like GPIOA, GPIOB:
#define GPIOA ((void*)0xA)
#define GPIOB ((void*)0xB)
#define GPIOC ((void*)0xC)

// GPIO Pin Definitions
#define GPIO_PIN_0                 ((uint16_t)0x0001)  /* Pin 0 selected    */
#define GPIO_PIN_1                 ((uint16_t)0x0002)  /* Pin 1 selected    */
#define GPIO_PIN_2                 ((uint16_t)0x0004)  /* Pin 2 selected    */
#define GPIO_PIN_3                 ((uint16_t)0x0008)  /* Pin 3 selected    */
#define GPIO_PIN_4                 ((uint16_t)0x0010)  /* Pin 4 selected    */
#define GPIO_PIN_5                 ((uint16_t)0x0020)  /* Pin 5 selected    */
#define GPIO_PIN_6                 ((uint16_t)0x0040)  /* Pin 6 selected    */
#define GPIO_PIN_7                 ((uint16_t)0x0080)  /* Pin 7 selected    */
#define GPIO_PIN_8                 ((uint16_t)0x0100)  /* Pin 8 selected    */
#define GPIO_PIN_9                 ((uint16_t)0x0200)  /* Pin 9 selected    */
#define GPIO_PIN_10                ((uint16_t)0x0400)  /* Pin 10 selected   */
#define GPIO_PIN_11                ((uint16_t)0x0800)  /* Pin 11 selected   */
#define GPIO_PIN_12                ((uint16_t)0x1000)  /* Pin 12 selected   */
#define GPIO_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected   */
#define GPIO_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected   */
#define GPIO_PIN_15                ((uint16_t)0x8000)  /* Pin 15 selected   */
#define GPIO_PIN_All               ((uint16_t)0xFFFF)  /* All pins selected */

#endif // MOCK_HAL_H
