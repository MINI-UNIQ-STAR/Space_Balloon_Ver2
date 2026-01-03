#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// CM1107N (Cubic) CO2 sensor
// - I2C 7-bit slave address: 0x31 (per spec)
// - Read measurement command: 0x01
// - Response: [0x01][DF0][DF1][STATUS][CS]
//   ppm = DF0*256 + DF1
//   CS = -([CMD]+DF0+DF1+STATUS) (keep lowest byte)

bool cm1107n_read_ppm(uint16_t *out_ppm, uint8_t *out_status);

#ifdef __cplusplus
}
#endif
