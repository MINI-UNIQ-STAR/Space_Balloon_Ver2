#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MS5611 I2C address (7-bit). Datasheet: 111011Cx where C = ~CSB.
// If CSB is tied to VDD => C=0 => 0x76. If CSB is tied to GND => C=1 => 0x77.
#ifndef MS5611_I2C_ADDR_7BIT
#define MS5611_I2C_ADDR_7BIT 0x76u
#endif

typedef enum {
	MS5611_OSR_256 = 256,
	MS5611_OSR_512 = 512,
	MS5611_OSR_1024 = 1024,
	MS5611_OSR_2048 = 2048,
	MS5611_OSR_4096 = 4096,
} ms5611_osr_t;

bool ms5611_reset(void);

bool ms5611_read_prom_word(uint8_t index, uint16_t *word);

bool ms5611_start_d1_conversion(ms5611_osr_t osr);
bool ms5611_start_d2_conversion(ms5611_osr_t osr);

bool ms5611_read_adc(uint32_t *value);

uint32_t ms5611_conversion_time_ms(ms5611_osr_t osr);

#ifdef __cplusplus
}
#endif
