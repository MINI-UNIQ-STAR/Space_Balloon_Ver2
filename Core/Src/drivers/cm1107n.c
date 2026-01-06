#include "drivers/cm1107n.h"

#include "stm32g4xx_hal.h"

#include "drivers/i2c_bus_lock.h"

extern I2C_HandleTypeDef hi2c3;

enum {
	CM1107N_I2C_ADDR_7BIT = 0x31,
	CM1107N_CMD_READ_PPM = 0x01,
};

static uint8_t cm1107n_checksum(const uint8_t *buf, size_t n_without_cs)
{
	uint32_t sum = 0;
	for (size_t i = 0; i < n_without_cs; i++) {
		sum += (uint32_t)buf[i];
	}
	return (uint8_t)(0u - (uint8_t)sum);
}

bool cm1107n_read_ppm(uint16_t *out_ppm, uint8_t *out_status)
{
	if ((out_ppm == NULL) || (out_status == NULL)) {
		return false;
	}

	if (!i2c_bus_take(&hi2c3, 0u)) {
		return false;
	}

	const uint16_t addr = (uint16_t)(CM1107N_I2C_ADDR_7BIT << 1);
	const uint8_t cmd = (uint8_t)CM1107N_CMD_READ_PPM;

	// Send command
	if (HAL_I2C_Master_Transmit(&hi2c3, addr, (uint8_t *)&cmd, 1, 50) != HAL_OK) {
		i2c_bus_give(&hi2c3);
		return false;
	}

	// Read 5-byte response
	uint8_t resp[5] = {0};
	if (HAL_I2C_Master_Receive(&hi2c3, addr, resp, sizeof(resp), 50) != HAL_OK) {
		i2c_bus_give(&hi2c3);
		return false;
	}

	i2c_bus_give(&hi2c3);

	if (resp[0] != cmd) {
		return false;
	}

	const uint8_t expected = cm1107n_checksum(resp, 4);
	if (resp[4] != expected) {
		return false;
	}

	*out_ppm = (uint16_t)((uint16_t)resp[1] * 256u + (uint16_t)resp[2]);
	*out_status = resp[3];
	return true;
}
