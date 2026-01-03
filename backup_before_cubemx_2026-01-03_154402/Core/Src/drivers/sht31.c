#include "drivers/sht31.h"

#include "drivers/sht31_codec.h"

#include "stm32g4xx_hal.h"

extern I2C_HandleTypeDef hi2c3;

static uint16_t addr8(void)
{
	return (uint16_t)(SHT31_I2C_ADDR_7BIT << 1u);
}

bool sht31_start_measurement(void)
{
	// Command: 0x2400 = single shot, high repeatability, clock stretching disabled
	uint8_t cmd[2] = {0x24u, 0x00u};
	return (HAL_I2C_Master_Transmit(&hi2c3, addr8(), cmd, sizeof(cmd), 10u) == HAL_OK);
}

bool sht31_read_raw(uint16_t *raw_t, uint16_t *raw_rh)
{
	uint8_t buf[6];
	if (HAL_I2C_Master_Receive(&hi2c3, addr8(), buf, sizeof(buf), 10u) != HAL_OK) {
		return false;
	}
	return sht31_parse_measurement(buf, raw_t, raw_rh);
}
