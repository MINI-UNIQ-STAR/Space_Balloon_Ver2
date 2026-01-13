#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/** @brief HAL 상태 열거형 */
typedef enum 
{
  HAL_OK       = 0x00U, /**< 정상 처리됨 */
  HAL_ERROR    = 0x01U, /**< 오류 발생 */
  HAL_BUSY     = 0x02U, /**< 리소스 사용 중 (Busy) */
  HAL_TIMEOUT  = 0x03U  /**< 시간 초과 */
} HAL_StatusTypeDef;

// CMSIS Mocks
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

extern uint32_t SystemCoreClock; /**< 시스템 코어 클럭 (Hz) */

/** @brief GPIO 핀 상태 */
typedef enum
{
  GPIO_PIN_RESET = 0u, /**< Low 상태 (0) */
  GPIO_PIN_SET         /**< High 상태 (1) */
} GPIO_PinState;

/** @brief GPIO 초기화 구조체 */
typedef struct
{
  uint32_t Pin;       /**< 설정할 GPIO 핀 번호 */
  uint32_t Mode;      /**< 동작 모드 (입력/출력 등) */
  uint32_t Pull;      /**< 풀업/풀다운 저항 설정 */
  uint32_t Speed;     /**< 출력 속도 설정 */
  uint32_t Alternate; /**< 대체 기능 (Alternate Function) 번호 */
} GPIO_InitTypeDef;

// GPIO Modes (매크로 정의)
#define GPIO_MODE_INPUT     0x00000000U /**< 입력 모드 */
#define GPIO_MODE_OUTPUT_PP 0x00000001U /**< 푸시-풀 출력 모드 */
#define GPIO_MODE_OUTPUT_OD 0x00000011U /**< 오픈-드레인 출력 모드 */
#define GPIO_NOPULL         0x00000000U /**< 저항 없음 */
#define GPIO_PULLUP         0x00000001U /**< 풀업 저항 */
#define GPIO_SPEED_FREQ_LOW  0x00000000U /**< 저속 출력 */
#define GPIO_SPEED_FREQ_HIGH 0x00000002U /**< 고속 출력 */

// GPIO 함수 프로토타입 (Mock)
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin);

// 시간 관리 (Mock)
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

// 초기화
HAL_StatusTypeDef HAL_Init(void);

// --- 주변장치 핸들 타입 (Mock) ---
/** @brief I2C 핸들 구조체 */
typedef struct {
    uint32_t Instance; /**< 하드웨어 인스턴스 주소 */
} I2C_HandleTypeDef;

/** @brief UART 핸들 구조체 */
typedef struct {
    uint32_t Instance; /**< 하드웨어 인스턴스 주소 */
} UART_HandleTypeDef;

/** @brief ADC 핸들 구조체 */
typedef struct {
    uint32_t Instance; /**< 하드웨어 인스턴스 주소 */
} ADC_HandleTypeDef;

/** @brief TIM 핸들 구조체 */
typedef struct {
    uint32_t Instance; /**< 하드웨어 인스턴스 주소 */
} TIM_HandleTypeDef;

// --- Helper Macros ---
#define I2C_MEMADD_SIZE_8BIT 0x01
#define I2C_MEMADD_SIZE_16BIT 0x02

// --- I2C 함수 프로토타입 ---
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);

// --- UART 함수 프로토타입 ---
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

// --- ADC 함수 프로토타입 ---
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc);

// --- Timer/PWM 함수 프로토타입 ---
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
void __HAL_TIM_SET_COMPARE(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t Compare);

// --- Mock 상태 접근 (Unit Test용) ---
/** @brief 마지막 I2C 쓰기 작업 기록 */
typedef struct {
    uint16_t addr;      /**< 디바이스 주소 */
    uint16_t reg;       /**< 레지스터 주소 */
    uint8_t data[256];  /**< 기록된 데이터 */
    uint16_t len;       /**< 데이터 길이 */
} MockI2C_LastWrite_t;

MockI2C_LastWrite_t* MockI2C_GetLastWrite(void);
void MockI2C_ClearStats(void);
void MockI2C_SetNextReadData(const uint8_t* data, uint16_t len);
void MockHAL_AdvanceTick(uint32_t ms);

// --- Mock UART 상태 ---
/** @brief 마지막 UART 송신 작업 기록 */
typedef struct {
    uint8_t data[256]; /**< 송신된 데이터 */
    uint16_t len;      /**< 데이터 길이 */
} MockUART_LastTx_t;

void MockUART_ClearStats(void);
MockUART_LastTx_t* MockUART_GetLastTx(void);

// Macro Mocks
#define UNUSED(x) ((void)(x))

// GPIO 포트 정의 (Mock 주소)
#define GPIOA ((void*)0xA)
#define GPIOB ((void*)0xB)
#define GPIOC ((void*)0xC)

// GPIO 핀 정의
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
