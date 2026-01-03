#pragma once

#include "stm32g4xx_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t addr;
} sen0321_t;

/**
 * @brief Initialize the SEN0321 Ozone sensor.
 * @param dev Pointer to the device structure.
 * @param hi2c Pointer to the I2C handle.
 * @return true if initialization successful, false otherwise.
 */
bool sen0321_init(sen0321_t *dev, I2C_HandleTypeDef *hi2c);

/**
 * @brief Read Ozone concentration in PPB.
 * @param dev Pointer to the device structure.
 * @param ppb Pointer to store the read PPB value.
 * @return true if read successful, false otherwise.
 */
bool sen0321_read_ppb(sen0321_t *dev, int16_t *ppb);

#ifdef __cplusplus
}
#endif
