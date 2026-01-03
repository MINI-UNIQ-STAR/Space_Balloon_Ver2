#include "drivers/lsm6dsv16x.h"

#include "stm32g4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

enum {
	LSM6DSV16X_REG_WHO_AM_I = 0x0Fu,
	LSM6DSV16X_WHO_AM_I_VALUE = 0x70u,

	LSM6DSV16X_REG_CTRL1 = 0x10u,
	LSM6DSV16X_REG_CTRL2 = 0x11u,
	LSM6DSV16X_REG_CTRL3 = 0x12u,
	LSM6DSV16X_REG_CTRL6 = 0x15u,
	LSM6DSV16X_REG_CTRL8 = 0x17u,

	LSM6DSV16X_REG_OUTX_L_G = 0x22u,

	LSM6DSV16X_TIMEOUT_MS = 10u,
};

static lsm6dsv16x_i2c_addr_t s_addr_7bit;
static bool s_inited;

static uint16_t addr8(lsm6dsv16x_i2c_addr_t a)
{
	return (uint16_t)((uint16_t)a << 1u);
}

static bool i2c_write_reg(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = {reg, value};
	return HAL_I2C_Master_Transmit(&hi2c1, addr8(s_addr_7bit), buf, sizeof(buf), LSM6DSV16X_TIMEOUT_MS) == HAL_OK;
}

static bool i2c_read_reg(uint8_t reg, uint8_t *out)
{
	if (out == NULL) {
		return false;
	}
	if (HAL_I2C_Master_Transmit(&hi2c1, addr8(s_addr_7bit), &reg, 1u, LSM6DSV16X_TIMEOUT_MS) != HAL_OK) {
		return false;
	}
	return HAL_I2C_Master_Receive(&hi2c1, addr8(s_addr_7bit), out, 1u, LSM6DSV16X_TIMEOUT_MS) == HAL_OK;
}

static bool i2c_read_burst(uint8_t start_reg, uint8_t *buf, uint16_t len)
{
	if ((buf == NULL) || (len == 0u)) {
		return false;
	}
	if (HAL_I2C_Master_Transmit(&hi2c1, addr8(s_addr_7bit), &start_reg, 1u, LSM6DSV16X_TIMEOUT_MS) != HAL_OK) {
		return false;
	}
	return HAL_I2C_Master_Receive(&hi2c1, addr8(s_addr_7bit), buf, len, LSM6DSV16X_TIMEOUT_MS) == HAL_OK;
}

static bool probe_addr(lsm6dsv16x_i2c_addr_t a)
{
	s_addr_7bit = a;
	uint8_t who = 0;
	if (!i2c_read_reg(LSM6DSV16X_REG_WHO_AM_I, &who)) {
		return false;
	}
	return who == LSM6DSV16X_WHO_AM_I_VALUE;
}

static uint8_t build_ctrl1(const lsm6dsv16x_config_t *cfg)
{
	// CTRL1 (0x10): [7]=0, [6:4]=OP_MODE_XL, [3:0]=ODR_XL
	// Use high-performance mode (OP_MODE_XL=000) unless the user wants something else later.
	const uint8_t op_mode_xl = 0u;
	return (uint8_t)((op_mode_xl << 4) | ((uint8_t)cfg->odr_xl & 0x0Fu));
}

static uint8_t build_ctrl2(const lsm6dsv16x_config_t *cfg)
{
	// CTRL2 (0x11): [7]=0, [6:4]=OP_MODE_G, [3:0]=ODR_G
	const uint8_t op_mode_g = 0u;
	return (uint8_t)((op_mode_g << 4) | ((uint8_t)cfg->odr_g & 0x0Fu));
}

static uint8_t build_ctrl3(void)
{
	// CTRL3 (0x12): BOOT(7), BDU(6), 0(5:3), IF_INC(2), 0(1), SW_RESET(0)
	const uint8_t boot = 0u;
	const uint8_t bdu = 1u;
	const uint8_t if_inc = 1u;
	const uint8_t sw_reset = 0u;
	return (uint8_t)((boot << 7) | (bdu << 6) | (if_inc << 2) | (sw_reset << 0));
}

static uint8_t build_ctrl6(const lsm6dsv16x_config_t *cfg)
{
	// CTRL6 (0x15): [7]=0, [6:4]=LPF1_G_BW, [3:0]=FS_G
	const uint8_t lpf1_g_bw = 0u;
	return (uint8_t)((lpf1_g_bw << 4) | ((uint8_t)cfg->fs_g & 0x0Fu));
}

static uint8_t build_ctrl8(const lsm6dsv16x_config_t *cfg)
{
	// CTRL8 (0x17): [7:5]=HP_LPF2_XL_BW, [4]=0, [3]=XL_DualC_EN, [2]=0, [1:0]=FS_XL
	const uint8_t hp_lpf2_xl_bw = 0u;
	const uint8_t xl_dualc_en = 0u;
	return (uint8_t)((hp_lpf2_xl_bw << 5) | (xl_dualc_en << 3) | ((uint8_t)cfg->fs_xl & 0x03u));
}

bool lsm6dsv16x_init(const lsm6dsv16x_config_t *cfg)
{
	if (cfg == NULL) {
		return false;
	}

	s_inited = false;

	// Probe address if not clearly specified.
	if ((cfg->i2c_addr == LSM6DSV16X_I2C_ADDR_7BIT_SA0_0) || (cfg->i2c_addr == LSM6DSV16X_I2C_ADDR_7BIT_SA0_1)) {
		if (!probe_addr(cfg->i2c_addr)) {
			return false;
		}
	} else {
		if (!probe_addr(LSM6DSV16X_I2C_ADDR_7BIT_SA0_0) && !probe_addr(LSM6DSV16X_I2C_ADDR_7BIT_SA0_1)) {
			return false;
		}
	}

	// Soft reset.
	// CTRL3.SW_RESET = 1 (auto-cleared)
	if (!i2c_write_reg(LSM6DSV16X_REG_CTRL3, 0x01u)) {
		return false;
	}
	HAL_Delay(20u);

	// Re-apply CTRL3 defaults we rely on: BDU=1, IF_INC=1.
	if (!i2c_write_reg(LSM6DSV16X_REG_CTRL3, build_ctrl3())) {
		return false;
	}

	// Full-scale configuration.
	if (!i2c_write_reg(LSM6DSV16X_REG_CTRL6, build_ctrl6(cfg))) {
		return false;
	}
	if (!i2c_write_reg(LSM6DSV16X_REG_CTRL8, build_ctrl8(cfg))) {
		return false;
	}

	// ODR/mode configuration (enables sensors).
	if (!i2c_write_reg(LSM6DSV16X_REG_CTRL1, build_ctrl1(cfg))) {
		return false;
	}
	if (!i2c_write_reg(LSM6DSV16X_REG_CTRL2, build_ctrl2(cfg))) {
		return false;
	}

	s_inited = true;
	return true;
}

bool lsm6dsv16x_read_accel_gyro_raw(int16_t out_accel_xyz[3], int16_t out_gyro_xyz[3])
{
	if ((!s_inited) || (out_accel_xyz == NULL) || (out_gyro_xyz == NULL)) {
		return false;
	}

	uint8_t buf[12];
	if (!i2c_read_burst(LSM6DSV16X_REG_OUTX_L_G, buf, sizeof(buf))) {
		return false;
	}

	// Gyro: 0x22..0x27 (X_L, X_H, Y_L, Y_H, Z_L, Z_H)
	out_gyro_xyz[0] = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
	out_gyro_xyz[1] = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
	out_gyro_xyz[2] = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));

	// Accel: 0x28..0x2D
	out_accel_xyz[0] = (int16_t)((uint16_t)buf[6] | ((uint16_t)buf[7] << 8));
	out_accel_xyz[1] = (int16_t)((uint16_t)buf[8] | ((uint16_t)buf[9] << 8));
	out_accel_xyz[2] = (int16_t)((uint16_t)buf[10] | ((uint16_t)buf[11] << 8));

	return true;
}
