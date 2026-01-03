#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// FTLAB GDK101 gamma radiation sensor module.
// I2C protocol (datasheet v1.5):
// - Default 7-bit address: 0x18 (+ optional A0/A1 user address bits)
// - Read measured value (1min avg / 1min update): CMD 0xB3
// - Read measured value (10min avg / 1min update): CMD 0xB2
// Response is 2 bytes: [integer][decimal], representing value = integer.decimal (uSv/h).

#ifndef GDK101_I2C_ADDR_7BIT
#define GDK101_I2C_ADDR_7BIT 0x18u
#endif

typedef enum {
	GDK101_CMD_RESET = 0xA0u,
	GDK101_CMD_STATUS_VIB = 0xB0u,
	GDK101_CMD_MEAS_TIME = 0xB1u,
	GDK101_CMD_MEAS_10MIN = 0xB2u,
	GDK101_CMD_MEAS_1MIN = 0xB3u,
	GDK101_CMD_FW_VERSION = 0xB4u,
} gdk101_cmd_t;

// Read latest gamma value as uSv/h * 100 (e.g. 1.21 uSv/h => 121).
// Returns false on I2C failure or invalid formatting.
bool gdk101_read_usvh_x100(uint16_t *out_usvh_x100, bool use_10min_avg);

// Read status and vibration status.
bool gdk101_read_status(uint8_t *out_status, uint8_t *out_vibration);

#ifdef __cplusplus
}
#endif
