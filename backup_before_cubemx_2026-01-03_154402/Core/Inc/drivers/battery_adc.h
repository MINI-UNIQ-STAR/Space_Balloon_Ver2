#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool valid;
	uint16_t adc_raw;     // 0..4095
	uint16_t vbat_mv;     // estimated battery voltage in millivolts
} battery_reading_t;

// Initializes ADC1 for battery measurement on PA1 (ADC1_IN2).
// Note: This project currently doesn't have CubeMX ADC init; this module sets up ADC1 internally.
bool battery_adc_init(void);

// Reads battery voltage.
// Returns true if a conversion succeeded.
bool battery_adc_read(battery_reading_t *out);

#ifdef __cplusplus
}
#endif
