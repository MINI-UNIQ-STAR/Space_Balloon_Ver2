#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Per-I2C-bus mutex to serialize access across tasks.
// NOTE: This does not make I2C preemptable; it prevents concurrent transactions.

void i2c_bus_lock_init(void);

bool i2c_bus_take(I2C_HandleTypeDef *hi2c, uint32_t timeout_ms);
void i2c_bus_give(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif
