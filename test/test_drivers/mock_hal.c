#include "mock_hal.h"

static uint32_t current_tick = 0;

// --- I2C Mock State ---
static MockI2C_LastWrite_t last_i2c_write;
static uint8_t next_read_data[256];
static uint16_t next_read_len = 0;

void MockI2C_ClearStats(void) {
    memset(&last_i2c_write, 0, sizeof(last_i2c_write));
    current_tick = 0;
    next_read_len = 0;
}

MockI2C_LastWrite_t* MockI2C_GetLastWrite(void) {
    return &last_i2c_write;
}

void MockI2C_SetNextReadData(const uint8_t* data, uint16_t len) {
    if (len > 256) len = 256;
    memcpy(next_read_data, data, len);
    next_read_len = len;
}

// --- Time ---
uint32_t HAL_GetTick(void) {
    return current_tick;
}

void HAL_Delay(uint32_t Delay) {
    current_tick += Delay;
}

void MockHAL_AdvanceTick(uint32_t ms) {
    current_tick += ms;
}

// --- I2C Implementations ---
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    last_i2c_write.addr = DevAddress;
    last_i2c_write.reg = MemAddress;
    last_i2c_write.len = Size;
    if (Size > 256) Size = 256;
    memcpy(last_i2c_write.data, pData, Size);
    
    printf("MOCK I2C WRITE: Addr=0x%02X, Reg=0x%02X, Len=%d\n", DevAddress, MemAddress, Size);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    printf("MOCK I2C READ: Addr=0x%02X, Reg=0x%02X, Len=%d\n", DevAddress, MemAddress, Size);
    
    // Return pre-set data if available, else 0
    if (next_read_len > 0) {
        // Copy min(Size, next_read_len)
        uint16_t copy_len = (Size < next_read_len) ? Size : next_read_len;
        memcpy(pData, next_read_data, copy_len);
    } else {
        memset(pData, 0, Size);
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    last_i2c_write.addr = DevAddress;
    last_i2c_write.reg = 0xFFFF; // Indicate raw transmit
    last_i2c_write.len = Size;
    if (Size > 256) Size = 256;
    memcpy(last_i2c_write.data, pData, Size);
    
    printf("MOCK I2C TX: Addr=0x%02X, Len=%d\n", DevAddress, Size);
    return HAL_OK;
}

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

// --- UART Implementations ---
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
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

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    return HAL_TIMEOUT; // Default to timeout/no data
}

// --- ADC Implementations ---
ADC_HandleTypeDef hadc1;
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc) { return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef* hadc, uint32_t Timeout) { return HAL_OK; }
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef* hadc) { return 2048; } // Mid-range
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc) { return HAL_OK; }

// --- GPIO ---
uint32_t SystemCoreClock = 16000000;

void HAL_GPIO_WritePin(void* GPIOx, uint16_t GPIO_Pin, int PinState) {
    if (GPIO_Pin == 1024) { // Filter spam for FDIR pin if needed, or just print
         // printf("MOCK GPIO WRITE: Port=%p, Pin=%d, State=%d\n", GPIOx, GPIO_Pin, PinState);
    }
}

int HAL_GPIO_ReadPin(void* GPIOx, uint16_t GPIO_Pin) {
    return 1; // Always return High/Set
}

void HAL_GPIO_Init(void* GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    // printf("MOCK GPIO INIT: Port=%p, Pin=%d\n", GPIOx, GPIO_Init->Pin);
}
