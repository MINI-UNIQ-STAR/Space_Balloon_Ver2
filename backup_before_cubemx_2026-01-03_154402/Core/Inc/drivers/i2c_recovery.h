#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Attempts to recover a stuck I2C bus by:
// - HAL_I2C_DeInit / HAL_I2C_Init
// - Bus clear: reconfigure SCL/SDA as GPIO open-drain and toggle SCL 9 times
// Returns true if the sequence completed and HAL_I2C_Init succeeded.
bool i2c_recover_bus(I2C_HandleTypeDef *hi2c,
                     GPIO_TypeDef *scl_port,
                     uint16_t scl_pin,
                     GPIO_TypeDef *sda_port,
                     uint16_t sda_pin);

#ifdef __cplusplus
}
#endif
