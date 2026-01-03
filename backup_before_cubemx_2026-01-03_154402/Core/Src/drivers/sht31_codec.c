#include "drivers/sht31_codec.h"

// Sensirion SHT3x CRC8:
// - Polynomial: 0x31 (x^8 + x^5 + x^4 + 1)
// - Init: 0xFF
// - No XOR out
uint8_t sht31_crc8(const uint8_t *data, size_t len)
{
	uint8_t crc = 0xFF;
	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t bit = 0; bit < 8u; bit++) {
			if ((crc & 0x80u) != 0u) {
				crc = (uint8_t)((crc << 1u) ^ 0x31u);
			} else {
				crc = (uint8_t)(crc << 1u);
			}
		}
	}
	return crc;
}

bool sht31_parse_measurement(const uint8_t buf[6], uint16_t *raw_t, uint16_t *raw_rh)
{
	if ((buf == NULL) || (raw_t == NULL) || (raw_rh == NULL)) {
		return false;
	}

	const uint8_t t_crc = sht31_crc8(&buf[0], 2u);
	const uint8_t rh_crc = sht31_crc8(&buf[3], 2u);
	if ((t_crc != buf[2]) || (rh_crc != buf[5])) {
		return false;
	}

	*raw_t = (uint16_t)(((uint16_t)buf[0] << 8u) | (uint16_t)buf[1]);
	*raw_rh = (uint16_t)(((uint16_t)buf[3] << 8u) | (uint16_t)buf[4]);
	return true;
}

int16_t sht31_temp_c_x100_from_raw(uint16_t raw_t)
{
	// T = -45 + 175 * raw / 65535
	// T_x100 = -4500 + 17500 * raw / 65535
	const int32_t num = (int32_t)17500 * (int32_t)raw_t + 32767; // rounding
	const int32_t div = num / 65535;
	return (int16_t)(-4500 + div);
}

uint16_t sht31_rh_x100_from_raw(uint16_t raw_rh)
{
	// RH = 100 * raw / 65535
	// RH_x100 = 10000 * raw / 65535
	const uint32_t num = (uint32_t)10000 * (uint32_t)raw_rh + 32767u; // rounding
	const uint32_t div = num / 65535u;
	if (div > 10000u) {
		return 10000u;
	}
	return (uint16_t)div;
}
