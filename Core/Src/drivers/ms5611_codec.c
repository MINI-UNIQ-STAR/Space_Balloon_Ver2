#include "drivers/ms5611_codec.h"

#include <math.h>
#include <stddef.h>

uint8_t ms5611_crc4(const uint16_t prom_in[8])
{
	// CRC4 algorithm described for MS56xx family (AN520).
	// Compute over 16 bytes (8 words), with CRC nibble cleared.
	uint16_t prom[8];
	for (int i = 0; i < 8; i++) {
		prom[i] = prom_in[i];
	}

	// Save and clear CRC nibble
	const uint16_t crc_read = (uint16_t)(prom[7] & 0x000Fu);
	(void)crc_read;
	prom[7] = (uint16_t)(prom[7] & 0xFF00u);

	// Some app notes also clear lower 12 bits of prom[0]; keep as-is except CRC nibble behavior
	uint16_t n_rem = 0;

	for (int cnt = 0; cnt < 16; cnt++) {
		uint8_t byte;
		if ((cnt & 1) == 0) {
			byte = (uint8_t)(prom[cnt >> 1] >> 8);
		} else {
			byte = (uint8_t)(prom[cnt >> 1] & 0x00FFu);
		}
		n_rem ^= (uint16_t)byte << 8;
		for (int n_bit = 0; n_bit < 8; n_bit++) {
			if ((n_rem & 0x8000u) != 0u) {
				n_rem = (uint16_t)((n_rem << 1u) ^ 0x3000u);
			} else {
				n_rem = (uint16_t)(n_rem << 1u);
			}
		}
	}

	return (uint8_t)((n_rem >> 12) & 0x0Fu);
}

bool ms5611_prom_crc_ok(const uint16_t prom[8])
{
	if (prom == NULL) {
		return false;
	}
	const uint8_t crc_expected = (uint8_t)(prom[7] & 0x0Fu);
	const uint8_t crc_calc = ms5611_crc4(prom);
	return (crc_calc == crc_expected);
}

bool ms5611_compensate(const uint16_t prom[8], uint32_t d1, uint32_t d2, int32_t *temp_c_x100, uint32_t *press_pa)
{
	if ((prom == NULL) || (temp_c_x100 == NULL) || (press_pa == NULL)) {
		return false;
	}

	// Coefficients C1..C6 are prom[1]..prom[6]
	const int64_t C1 = (int64_t)prom[1];
	const int64_t C2 = (int64_t)prom[2];
	const int64_t C3 = (int64_t)prom[3];
	const int64_t C4 = (int64_t)prom[4];
	const int64_t C5 = (int64_t)prom[5];
	const int64_t C6 = (int64_t)prom[6];

	const int64_t D1 = (int64_t)d1;
	const int64_t D2 = (int64_t)d2;

	// dT = D2 - C5 * 2^8
	const int64_t dT = D2 - (C5 << 8);

	// TEMP = 2000 + dT * C6 / 2^23   (0.01C)
	int64_t TEMP = 2000 + ((dT * C6) >> 23);

	// OFF = C2 * 2^16 + (C4 * dT) / 2^7
	int64_t OFF = (C2 << 16) + ((C4 * dT) >> 7);

	// SENS = C1 * 2^15 + (C3 * dT) / 2^8
	int64_t SENS = (C1 << 15) + ((C3 * dT) >> 8);

	// 2nd order compensation
	int64_t T2 = 0;
	int64_t OFF2 = 0;
	int64_t SENS2 = 0;

	if (TEMP < 2000) {
		// Low temperature
		const int64_t temp_minus_2000 = TEMP - 2000;
		T2 = (dT * dT) >> 31;
		OFF2 = (5 * (temp_minus_2000 * temp_minus_2000)) >> 1;
		SENS2 = (5 * (temp_minus_2000 * temp_minus_2000)) >> 2;

		if (TEMP < -1500) {
			// Very low temperature
			const int64_t temp_plus_1500 = TEMP + 1500;
			OFF2 += 7 * (temp_plus_1500 * temp_plus_1500);
			SENS2 += (11 * (temp_plus_1500 * temp_plus_1500)) >> 1;
		}
	}

	TEMP -= T2;
	OFF -= OFF2;
	SENS -= SENS2;

	// P = (D1 * SENS / 2^21 - OFF) / 2^15
	const int64_t P = (((D1 * SENS) >> 21) - OFF) >> 15;

	*temp_c_x100 = (int32_t)TEMP;
	if (P < 0) {
		*press_pa = 0u;
	} else {
		*press_pa = (uint32_t)P;
	}
	return true;
}

int32_t ms5611_altitude_m_from_pressure(uint32_t press_pa, uint32_t p0_pa)
{
	if ((press_pa == 0u) || (p0_pa == 0u)) {
		return 0;
	}

	// Standard atmosphere approximation:
	// h = 44330 * (1 - (P / P0)^(1/5.255))
	// Use float internally; wire format remains integer.
	const float ratio = (float)press_pa / (float)p0_pa;
	if (ratio <= 0.0f) {
		return 0;
	}
	const float exponent = 0.190294957f; // 1/5.255
	const float h_m = 44330.0f * (1.0f - powf(ratio, exponent));
	// Round to nearest meter.
	const long h_m_i = lroundf(h_m);
	if (h_m_i > INT32_MAX) {
		return INT32_MAX;
	}
	if (h_m_i < INT32_MIN) {
		return INT32_MIN;
	}
	return (int32_t)h_m_i;
}
