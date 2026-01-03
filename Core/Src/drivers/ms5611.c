#include "drivers/ms5611.h"

#include "stm32g4xx_hal.h"

#include "drivers/i2c_bus_lock.h"

extern I2C_HandleTypeDef hi2c3;

static uint16_t addr8(void)
{
	return (uint16_t)(MS5611_I2C_ADDR_7BIT << 1u);
}

static uint8_t osr_to_cmd_bits(ms5611_osr_t osr)
{
	switch (osr) {
	case MS5611_OSR_256:
		return 0x00u;
	case MS5611_OSR_512:
		return 0x02u;
	case MS5611_OSR_1024:
		return 0x04u;
	case MS5611_OSR_2048:
		return 0x06u;
	case MS5611_OSR_4096:
	default:
		return 0x08u;
	}
}

bool ms5611_reset(void)
{
	uint8_t cmd = 0x1Eu;
	if (!i2c_bus_take(&hi2c3, 1u)) {
		return false;
	}
	const bool ok = (HAL_I2C_Master_Transmit(&hi2c3, addr8(), &cmd, 1u, 10u) == HAL_OK);
	i2c_bus_give(&hi2c3);
	return ok;
}

bool ms5611_read_prom_word(uint8_t index, uint16_t *word)
{
	if ((word == NULL) || (index > 7u)) {
		return false;
	}
	uint8_t cmd = (uint8_t)(0xA0u + (uint8_t)(index * 2u));
	if (!i2c_bus_take(&hi2c3, 2u)) {
		return false;
	}
	uint8_t buf[2];
	bool ok = true;
	if (HAL_I2C_Master_Transmit(&hi2c3, addr8(), &cmd, 1u, 10u) != HAL_OK) {
		ok = false;
	}
	if (ok && (HAL_I2C_Master_Receive(&hi2c3, addr8(), buf, 2u, 10u) != HAL_OK)) {
		ok = false;
	}
	i2c_bus_give(&hi2c3);
	if (!ok) {
		return false;
	}
	*word = (uint16_t)(((uint16_t)buf[0] << 8u) | (uint16_t)buf[1]);
	return true;
}

bool ms5611_start_d1_conversion(ms5611_osr_t osr)
{
	uint8_t cmd = (uint8_t)(0x40u | osr_to_cmd_bits(osr));
	if (!i2c_bus_take(&hi2c3, 1u)) {
		return false;
	}
	const bool ok = (HAL_I2C_Master_Transmit(&hi2c3, addr8(), &cmd, 1u, 10u) == HAL_OK);
	i2c_bus_give(&hi2c3);
	return ok;
}

bool ms5611_start_d2_conversion(ms5611_osr_t osr)
{
	uint8_t cmd = (uint8_t)(0x50u | osr_to_cmd_bits(osr));
	if (!i2c_bus_take(&hi2c3, 1u)) {
		return false;
	}
	const bool ok = (HAL_I2C_Master_Transmit(&hi2c3, addr8(), &cmd, 1u, 10u) == HAL_OK);
	i2c_bus_give(&hi2c3);
	return ok;
}

bool ms5611_read_adc(uint32_t *value)
{
	if (value == NULL) {
		return false;
	}
	uint8_t cmd = 0x00u;
	if (!i2c_bus_take(&hi2c3, 2u)) {
		return false;
	}
	uint8_t buf[3];
	bool ok = true;
	if (HAL_I2C_Master_Transmit(&hi2c3, addr8(), &cmd, 1u, 10u) != HAL_OK) {
		ok = false;
	}
	if (ok && (HAL_I2C_Master_Receive(&hi2c3, addr8(), buf, 3u, 10u) != HAL_OK)) {
		ok = false;
	}
	i2c_bus_give(&hi2c3);
	if (!ok) {
		return false;
	}
	*value = ((uint32_t)buf[0] << 16u) | ((uint32_t)buf[1] << 8u) | (uint32_t)buf[2];
	return true;
}

uint32_t ms5611_conversion_time_ms(ms5611_osr_t osr)
{
	// Use max conversion time (datasheet):
	// 4096: 9.04ms, 2048: 4.54ms, 1024: 2.28ms, 512: 1.17ms, 256: 0.60ms
	switch (osr) {
	case MS5611_OSR_256:
		return 1u;
	case MS5611_OSR_512:
		return 2u;
	case MS5611_OSR_1024:
		return 3u;
	case MS5611_OSR_2048:
		return 5u;
	case MS5611_OSR_4096:
	default:
		return 10u;
	}
}
