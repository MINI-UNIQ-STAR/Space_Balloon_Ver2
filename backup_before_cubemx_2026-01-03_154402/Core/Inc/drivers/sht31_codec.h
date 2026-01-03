#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// SHT31 measurement block: [T_MSB T_LSB T_CRC RH_MSB RH_LSB RH_CRC]

uint8_t sht31_crc8(const uint8_t *data, size_t len);

bool sht31_parse_measurement(const uint8_t buf[6], uint16_t *raw_t, uint16_t *raw_rh);

// Fixed-point conversions (SI-ish):
// - Temperature: degC * 100
// - Relative humidity: %RH * 100
int16_t sht31_temp_c_x100_from_raw(uint16_t raw_t);
uint16_t sht31_rh_x100_from_raw(uint16_t raw_rh);

#ifdef __cplusplus
}
#endif
