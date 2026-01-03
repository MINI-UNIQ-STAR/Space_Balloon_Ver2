#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ST LSM6DSV16x IMU (I2C1)
// Datasheet (DS13510 Rev 4) key registers:
// - WHO_AM_I: 0x0F (fixed value 0x70)
// - OUTX_L_G: 0x22 (gyro XYZ, 6 bytes)
// - OUTX_L_A: 0x28 (accel XYZ, 6 bytes)
// - CTRL1: 0x10 (OP_MODE_XL + ODR_XL)
// - CTRL2: 0x11 (OP_MODE_G + ODR_G)
// - CTRL3: 0x12 (BDU, IF_INC, SW_RESET)
// - CTRL6: 0x15 (FS_G)
// - CTRL8: 0x17 (FS_XL)

typedef enum {
	LSM6DSV16X_I2C_ADDR_7BIT_SA0_0 = 0x6Au,
	LSM6DSV16X_I2C_ADDR_7BIT_SA0_1 = 0x6Bu,
} lsm6dsv16x_i2c_addr_t;

typedef enum {
	LSM6DSV16X_FS_XL_2G  = 0u,
	LSM6DSV16X_FS_XL_4G  = 1u,
	LSM6DSV16X_FS_XL_8G  = 2u,
	LSM6DSV16X_FS_XL_16G = 3u,
} lsm6dsv16x_fs_xl_t;

typedef enum {
	LSM6DSV16X_FS_G_125DPS  = 0x0u,
	LSM6DSV16X_FS_G_250DPS  = 0x1u,
	LSM6DSV16X_FS_G_500DPS  = 0x2u,
	LSM6DSV16X_FS_G_1000DPS = 0x3u,
	LSM6DSV16X_FS_G_2000DPS = 0x4u,
	LSM6DSV16X_FS_G_4000DPS = 0xCu,
} lsm6dsv16x_fs_g_t;

typedef enum {
	LSM6DSV16X_ODR_POWER_DOWN = 0x0u,
	LSM6DSV16X_ODR_1P875_HZ   = 0x1u,
	LSM6DSV16X_ODR_7P5_HZ     = 0x2u,
	LSM6DSV16X_ODR_15_HZ      = 0x3u,
	LSM6DSV16X_ODR_30_HZ      = 0x4u,
	LSM6DSV16X_ODR_60_HZ      = 0x5u,
	LSM6DSV16X_ODR_120_HZ     = 0x6u,
	LSM6DSV16X_ODR_240_HZ     = 0x7u,
	LSM6DSV16X_ODR_480_HZ     = 0x8u,
	LSM6DSV16X_ODR_960_HZ     = 0x9u,
	LSM6DSV16X_ODR_1P92_KHZ   = 0xAu,
	LSM6DSV16X_ODR_3P84_KHZ   = 0xBu,
	LSM6DSV16X_ODR_7P68_KHZ   = 0xCu,
} lsm6dsv16x_odr_t;

typedef struct {
	lsm6dsv16x_i2c_addr_t i2c_addr;
	lsm6dsv16x_odr_t odr_xl;
	lsm6dsv16x_odr_t odr_g;
	lsm6dsv16x_fs_xl_t fs_xl;
	lsm6dsv16x_fs_g_t fs_g;
} lsm6dsv16x_config_t;

// Initializes the device and applies configuration.
// If cfg->i2c_addr is not one of the known addresses, init will probe both 0x6A and 0x6B using WHO_AM_I.
bool lsm6dsv16x_init(const lsm6dsv16x_config_t *cfg);

// Reads both gyro and accel in one burst read starting at OUTX_L_G (0x22).
// Outputs are signed raw counts (little-endian registers).
bool lsm6dsv16x_read_accel_gyro_raw(int16_t out_accel_xyz[3], int16_t out_gyro_xyz[3]);

#ifdef __cplusplus
}
#endif
