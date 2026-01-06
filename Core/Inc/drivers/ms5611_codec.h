#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// PROM layout: 8 words (0..7). Words 1..6 contain C1..C6.
// Word 7 low nibble contains factory CRC (CRC4).

typedef struct {
	uint16_t prom[8];
} ms5611_prom_t;

uint8_t ms5611_crc4(const uint16_t prom[8]);

bool ms5611_prom_crc_ok(const uint16_t prom[8]);

// Compute compensated temperature and pressure.
// Outputs:
// - temp_c_x100: degC * 100
// - press_pa: Pascal (note: datasheet pressure output in 0.01 mbar equals Pa numerically)
// Returns false if inputs are invalid.
bool ms5611_compensate(const uint16_t prom[8], uint32_t d1, uint32_t d2, int32_t *temp_c_x100, uint32_t *press_pa);

// Compute barometric altitude from pressure using the standard atmosphere approximation.
// - p0_pa: sea-level reference pressure (Pa), typically 101325.
// - Returns altitude in millimeters.
int32_t ms5611_altitude_m_from_pressure(uint32_t press_pa, uint32_t p0_pa);

#ifdef __cplusplus
}
#endif
