#include "drivers/gdk101.h"

#include "stm32g4xx_hal.h"

#include "drivers/i2c_bus_lock.h"

extern I2C_HandleTypeDef hi2c1;

static uint16_t addr8(void)
{
	return (uint16_t)(GDK101_I2C_ADDR_7BIT << 1u);
}

static bool read_u16_decimal(uint8_t cmd, uint16_t *out_x100)
{
	if (out_x100 == NULL) {
		return false;
	}

	if (!i2c_bus_take(&hi2c1, 0u)) {
		return false;
	}
	uint8_t buf[2];
	bool ok = true;
	if (HAL_I2C_Master_Transmit(&hi2c1, addr8(), &cmd, 1u, 10u) != HAL_OK) {
		ok = false;
	}
	if (ok && (HAL_I2C_Master_Receive(&hi2c1, addr8(), buf, sizeof(buf), 10u) != HAL_OK)) {
		ok = false;
	}
	i2c_bus_give(&hi2c1);
	if (!ok) {
		return false;
	}

	const uint16_t integer_part = (uint16_t)buf[0];
	const uint16_t decimal_part = (uint16_t)buf[1];
	if (decimal_part > 99u) {
		return false;
	}

	*out_x100 = (uint16_t)(integer_part * 100u + decimal_part);
	return true;
}

bool gdk101_read_usvh_x100(uint16_t *out_usvh_x100, bool use_10min_avg)
{
	return read_u16_decimal(use_10min_avg ? (uint8_t)GDK101_CMD_MEAS_10MIN : (uint8_t)GDK101_CMD_MEAS_1MIN,
						out_usvh_x100);
}

bool gdk101_read_status(uint8_t *out_status, uint8_t *out_vibration)
{
	if ((out_status == NULL) || (out_vibration == NULL)) {
		return false;
	}

	uint8_t cmd = (uint8_t)GDK101_CMD_STATUS_VIB;
	if (!i2c_bus_take(&hi2c1, 0u)) {
		return false;
	}
	uint8_t buf[2];
	bool ok = true;
	if (HAL_I2C_Master_Transmit(&hi2c1, addr8(), &cmd, 1u, 10u) != HAL_OK) {
		ok = false;
	}
	if (ok && (HAL_I2C_Master_Receive(&hi2c1, addr8(), buf, sizeof(buf), 10u) != HAL_OK)) {
		ok = false;
	}
	i2c_bus_give(&hi2c1);
	if (!ok) {
		return false;
	}

	*out_status = buf[0];
	*out_vibration = buf[1];
	return true;
}
