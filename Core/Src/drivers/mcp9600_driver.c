#include "drivers/mcp9600_driver.h"

#include "stm32g4xx_hal.h"

#include "drivers/i2c_bus_lock.h"

extern I2C_HandleTypeDef hi2c3;

enum {
	MCP9600_I2C_ADDR_7BIT = 0x67,
	MCP9600_REG_HOT_JUNCTION = 0x00,   // Thermocouple (external) temperature
	MCP9600_REG_COLD_JUNCTION = 0x02,  // Ambient (internal) temperature
	MCP9600_TEMP_LSB_C_X100_NUM = 25,  // 0.0625C = 25/400 C => 25/4 centiC
	MCP9600_TEMP_LSB_C_X100_DEN = 4,
};

static int32_t convert_raw_to_c_x100(int16_t raw)
{
	// Datasheet/lib convention: signed 16-bit, 0.0625°C per LSB.
	// Convert to centi-degC using integer math: raw * (100/16) = raw * 25 / 4.
	int32_t scaled = (int32_t)raw * MCP9600_TEMP_LSB_C_X100_NUM;
	if (scaled >= 0) {
		return (scaled + (MCP9600_TEMP_LSB_C_X100_DEN / 2)) / MCP9600_TEMP_LSB_C_X100_DEN;
	}
	return (scaled - (MCP9600_TEMP_LSB_C_X100_DEN / 2)) / MCP9600_TEMP_LSB_C_X100_DEN;
}

bool mcp9600_read_cold_junction_c_x100(int32_t *out_c_x100)
{
	if (out_c_x100 == NULL) {
		return false;
	}

	if (!i2c_bus_take(&hi2c3, 0u)) {
		return false;
	}

	uint8_t buf[2] = {0};
	const uint16_t addr = (uint16_t)(MCP9600_I2C_ADDR_7BIT << 1);

	if (HAL_I2C_Mem_Read(&hi2c3,
					addr,
					MCP9600_REG_COLD_JUNCTION,
					I2C_MEMADD_SIZE_8BIT,
					buf,
					sizeof(buf),
					50) != HAL_OK) {
		i2c_bus_give(&hi2c3);
		return false;
	}

	i2c_bus_give(&hi2c3);

	uint16_t raw_u16 = (uint16_t)((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
	int16_t raw = (int16_t)raw_u16;
	*out_c_x100 = convert_raw_to_c_x100(raw);
	return true;
}

bool mcp9600_read_hot_junction_c_x100(int32_t *out_c_x100)
{
	if (out_c_x100 == NULL) {
		return false;
	}

	if (!i2c_bus_take(&hi2c3, 0u)) {
		return false;
	}

	uint8_t buf[2] = {0};
	const uint16_t addr = (uint16_t)(MCP9600_I2C_ADDR_7BIT << 1);

	if (HAL_I2C_Mem_Read(&hi2c3,
					addr,
					MCP9600_REG_HOT_JUNCTION,
					I2C_MEMADD_SIZE_8BIT,
					buf,
					sizeof(buf),
					50) != HAL_OK) {
		i2c_bus_give(&hi2c3);
		return false;
	}

	i2c_bus_give(&hi2c3);

	uint16_t raw_u16 = (uint16_t)((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
	int16_t raw = (int16_t)raw_u16;
	*out_c_x100 = convert_raw_to_c_x100(raw);
	return true;
}
