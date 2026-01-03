#include "drivers/mcp9600_probe.h"

#include "stm32g4xx_hal.h"

extern I2C_HandleTypeDef hi2c3;

// From reference/MCP9600 Adafruit guide: default I2C address is 0x67.
enum {
	MCP9600_I2C_ADDR_7BIT = 0x67,
};

bool mcp9600_probe_is_ready(void)
{
	const uint16_t addr = (uint16_t)(MCP9600_I2C_ADDR_7BIT << 1);
	return (HAL_I2C_IsDeviceReady(&hi2c3, addr, 1, 50) == HAL_OK);
}
