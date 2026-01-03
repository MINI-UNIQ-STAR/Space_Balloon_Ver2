#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool valid;
	int16_t temp_c_x100;
} ds18b20_reading_t;

// Initializes DS18B20 1-wire bus support.
// This does not block for conversion.
void ds18b20_init(void);

// Starts a temperature conversion (non-blocking).
// Returns true if the command was issued successfully.
bool ds18b20_start_conversion(void);

// Tries to read temperature.
// Returns true and fills out if a valid reading was obtained.
bool ds18b20_read_temperature(ds18b20_reading_t *out);

#ifdef __cplusplus
}
#endif
