#ifndef OWHAL_H
#define OWHAL_H

#include "main.h"

// Hardware configuration
#define OW_PIN  DS18B20_Pin
#define OW_PORT DS18B20_GPIO_Port

// Mode switching functions should be implemented or simple direction macros if possible
// For simple bit banging on STM32:
// 1. Output Mode (Init with Open Drain usually, but PushPull if direction switching)
// 2. Input Mode

__STATIC_INLINE void OW_DIR_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = OW_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // Open Drain for 1-Wire
    GPIO_InitStruct.Pull = GPIO_NOPULL; // External pull-up assumed, or PULLUP
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OW_PORT, &GPIO_InitStruct);
}

__STATIC_INLINE void OW_DIR_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = OW_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(OW_PORT, &GPIO_InitStruct);
}

#define OW_OUTPUT() OW_DIR_OUT()
#define OW_INPUT()  OW_DIR_IN()

#define OW_LOW()    HAL_GPIO_WritePin(OW_PORT, OW_PIN, GPIO_PIN_RESET)
#define OW_HIGH()   HAL_GPIO_WritePin(OW_PORT, OW_PIN, GPIO_PIN_SET)
#define OW_READ()   (HAL_GPIO_ReadPin(OW_PORT, OW_PIN) == GPIO_PIN_SET)

// Simple delay loop - Calibrate for SystemCoreClock (Example for ~170MHz)
// 1 us ~ 170 cycles. Loop takes ~4 cycles? -> 40 iterations
__STATIC_INLINE void __delay_us(uint32_t us)
{
    volatile uint32_t count = us * (SystemCoreClock / 4000000); // approx
    while(count--) {
        __asm("nop");
    }
}

#endif // OWHAL_H
