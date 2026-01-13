#include "mock_hal.h"
#include <string.h>

/** @brief 현재 시스템 틱 (시뮬레이션 시간) */
static uint32_t current_tick = 0;

// --- I2C Mock 상태 ---
/** @brief 마지막 I2C 쓰기 작업 기록 */
static MockI2C_LastWrite_t last_i2c_write;
/** @brief 다음 I2C 읽기 요청 시 반환할 미리 설정된 데이터 */
static uint8_t next_read_data[256];
/** @brief 설정된 반환 데이터 길이 */
static uint16_t next_read_len = 0;

/** @brief I2C Mock 상태 및 통계 초기화 */
void MockI2C_ClearStats(void) {
    memset(&last_i2c_write, 0, sizeof(last_i2c_write));
    current_tick = 0;
    next_read_len = 0;
}

/** @brief 마지막 I2C 쓰기 정보 가져오기 */
MockI2C_LastWrite_t* MockI2C_GetLastWrite(void) {
    return &last_i2c_write;
}

/** 
 * @brief 다음 I2C 읽기 동작에 대한 응답 데이터 설정
 * @param data 반환할 데이터 배열
 * @param len 데이터 길이
 */
void MockI2C_SetNextReadData(const uint8_t* data, uint16_t len) {
    if (len > 256) len = 256;
    memcpy(next_read_data, data, len);
    next_read_len = len;
}

// --- Time 관련 함수 ---

/** @brief 시스템 틱 가져오기 (Mock) */
uint32_t HAL_GetTick(void) {
    return current_tick;
}

/** @brief 지연 시간 (Mock) - 틱 증가 */
void HAL_Delay(uint32_t Delay) {
    current_tick += Delay;
}

/** @brief 시간 강제 전진 (Test Helper) */
void MockHAL_AdvanceTick(uint32_t ms) {
    current_tick += ms;
}

// --- I2C Implementation (Mock) ---

/** @brief I2C 메모리 쓰기 (Mock) */
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    last_i2c_write.addr = DevAddress;
    last_i2c_write.reg = MemAddress;
    last_i2c_write.len = Size;
    if (Size > 256) Size = 256;
    memcpy(last_i2c_write.data, pData, Size);
    
    printf("MOCK I2C WRITE: Addr=0x%02X, Reg=0x%02X, Len=%d\n", DevAddress, MemAddress, Size);
    return HAL_OK;
}

/** @brief I2C 메모리 읽기 (Mock) */
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    printf("MOCK I2C READ: Addr=0x%02X, Reg=0x%02X, Len=%d\n", DevAddress, MemAddress, Size);
    
    // 미리 설정된 데이터가 있으면 반환, 없으면 0으로 채움
    if (next_read_len > 0) {
        // Size와 next_read_len 중 작은 값만큼 복사
        uint16_t copy_len = (Size < next_read_len) ? Size : next_read_len;
        memcpy(pData, next_read_data, copy_len);
    } else {
        memset(pData, 0, Size);
    }
    return HAL_OK;
}

/** @brief I2C 마스터 전송 (Mock) */
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    last_i2c_write.addr = DevAddress;
    last_i2c_write.reg = 0xFFFF; // 레지스터 주소 없음 (Raw Transmit)
    last_i2c_write.len = Size;
    if (Size > 256) Size = 256;
    memcpy(last_i2c_write.data, pData, Size);
    
    printf("MOCK I2C TX: Addr=0x%02X, Len=%d\n", DevAddress, Size);
    return HAL_OK;
}

/** @brief I2C 마스터 수신 (Mock) */
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    printf("MOCK I2C RX: Addr=0x%02X, Len=%d\n", DevAddress, Size);
    if (next_read_len > 0) {
        uint16_t copy_len = (Size < next_read_len) ? Size : next_read_len;
        memcpy(pData, next_read_data, copy_len);
    } else {
        memset(pData, 0, Size);
    }
    return HAL_OK;
}

// --- UART Mock State ---
static MockUART_LastTx_t last_uart_tx;

/** @brief UART 통계 초기화 */
void MockUART_ClearStats(void) {
    memset(&last_uart_tx, 0, sizeof(last_uart_tx));
}

/** @brief 마지막 UART 송신 정보 가져오기 */
MockUART_LastTx_t* MockUART_GetLastTx(void) {
    return &last_uart_tx;
}

// --- UART Implementations ---

/** @brief UART 전송 (Mock) */
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    if (Size > 256) Size = 256;
    memcpy(last_uart_tx.data, pData, Size);
    last_uart_tx.len = Size;
    
    char tmp[128];
    if (Size < 128) {
        memcpy(tmp, pData, Size);
        tmp[Size] = 0;
        printf("MOCK UART TX: %s\n", tmp);
    } else {
        printf("MOCK UART TX: [Binary %d bytes]\n", Size);
    }
    return HAL_OK;
}

/** @brief UART 수신 (Mock) - 항상 Timeout */
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_TIMEOUT; // 데이터 없음 시뮬레이션
}

// --- ADC Implementations ---
ADC_HandleTypeDef hadc1;
/** @brief ADC 시작 (Mock) */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc) { return HAL_OK; }
/** @brief ADC 변환 대기 (Mock) */
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout) { return HAL_OK; }
/** @brief ADC 값 읽기 (Mock) - 중간값(2048) 반환 */
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc) { return 2048; } // Mid-range
/** @brief ADC 중지 (Mock) */
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc) { return HAL_OK; }

// --- GPIO ---
uint32_t SystemCoreClock = 16000000;

/** @brief GPIO 핀 쓰기 (Mock) */
void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    if (GPIO_Pin == 1024) { // FDIR 핀 스팸 필터링 (필요 시)
         // printf("MOCK GPIO WRITE: Port=%p, Pin=%d, State=%d\n", GPIOx, GPIO_Pin, PinState);
    }
}

/** @brief GPIO 핀 읽기 (Mock) - 항상 HIGH */
GPIO_PinState HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin) {
    return GPIO_PIN_SET; // Always return High/Set
}

/** @brief GPIO 초기화 (Mock) */
void HAL_GPIO_Init(void* GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    // printf("MOCK GPIO INIT: Port=%p, Pin=%d\n", GPIOx, GPIO_Init->Pin);
}
