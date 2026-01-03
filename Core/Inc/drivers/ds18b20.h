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

// Initializes DS18B20 1-wire bus support and scans for devices.
void ds18b20_init(void);

// Scans the bus for devices. Returns true if at least one device found.
bool ds18b20_scan(void);

// Returns the number of devices found during the last scan.
int ds18b20_get_device_count(void);

// Starts a temperature conversion on ALL devices (Skip ROM).
// Returns true if the command was issued successfully.
bool ds18b20_start_conversion_all(void);

// Tries to read temperature from a specific device index.
// Returns true and fills out if a valid reading was obtained.
bool ds18b20_read_temperature(int index, ds18b20_reading_t *out);

#ifdef __cplusplus
}
#endif
