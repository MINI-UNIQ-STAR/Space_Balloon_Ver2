#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SHT31-D default I2C address (7-bit): 0x44 (ADDR pin low)
#ifndef SHT31_I2C_ADDR_7BIT
#define SHT31_I2C_ADDR_7BIT 0x44u
#endif

// Start a single-shot measurement (high repeatability, no clock stretching).
bool sht31_start_measurement(void);

// Read raw measurement words (validates CRC). Returns false on I2C/CRC failure.
bool sht31_read_raw(uint16_t *raw_t, uint16_t *raw_rh);

#ifdef __cplusplus
}
#endif
