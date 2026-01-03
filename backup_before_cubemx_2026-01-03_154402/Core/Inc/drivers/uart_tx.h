#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

bool uart1_tx_write(const uint8_t *data, size_t len, uint32_t timeout_ms);
bool uart3_tx_write(const uint8_t *data, size_t len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
