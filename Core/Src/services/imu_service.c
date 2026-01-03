#include "services/imu_service.h"

#include "drivers/lsm6dsv16x.h"

#include "stm32g4xx_hal.h"

enum {
	IMU_SAMPLE_PERIOD_MS = 20u, // 50 Hz, aligned with telemetry
};

// Sensitivities from LSM6DSV16x datasheet (DS13510 Rev 4, Mechanical characteristics):
// - LA_So: mg/LSB
// - G_So:  mdps/LSB

enum {
	G_MPS2_X1000 = 9807, // 9.80665 * 1000 rounded
	RAD_PER_DEG_Q1E9 = 17453293, // (pi/180) * 1e9 rounded
};

static uint32_t s_next_sample_ms;
static bool s_valid;
static int32_t s_accel_mps2_x1000[3];
static int32_t s_gyro_rads_x1000[3];
static uint32_t s_last_update_ms;

static uint16_t accel_mg_per_lsb_x1000(lsm6dsv16x_fs_xl_t fs)
{
	switch (fs) {
	case LSM6DSV16X_FS_XL_2G:
		return 61u; // 0.061 mg/LSB
	case LSM6DSV16X_FS_XL_4G:
		return 122u; // 0.122 mg/LSB
	case LSM6DSV16X_FS_XL_8G:
		return 244u; // 0.244 mg/LSB
	case LSM6DSV16X_FS_XL_16G:
		return 488u; // 0.488 mg/LSB
	default:
		return 61u;
	}
}

static uint32_t gyro_mdps_per_lsb(lsm6dsv16x_fs_g_t fs)
{
	switch (fs) {
	case LSM6DSV16X_FS_G_125DPS:
		return 4375u; // 4.375 mdps/LSB
	case LSM6DSV16X_FS_G_250DPS:
		return 8750u; // 8.75 mdps/LSB
	case LSM6DSV16X_FS_G_500DPS:
		return 17500u; // 17.50 mdps/LSB
	case LSM6DSV16X_FS_G_1000DPS:
		return 35000u; // 35 mdps/LSB
	case LSM6DSV16X_FS_G_2000DPS:
		return 70000u; // 70 mdps/LSB
	case LSM6DSV16X_FS_G_4000DPS:
		return 140000u; // 140 mdps/LSB
	default:
		return 70000u;
	}
}

static int32_t accel_raw_to_mps2_x1000(int16_t raw, uint16_t mg_per_lsb_x1000)
{
	// accel_mps2_x1000 = raw * (mg/LSB) * 9.80665
	// with mg_per_lsb_x1000 = (mg/LSB)*1000 and g_mps2_x1000 = 9.80665*1000:
	// accel_mps2_x1000 = raw * mg_per_lsb_x1000 * g_mps2_x1000 / 1e6
	int64_t tmp = (int64_t)raw * (int64_t)mg_per_lsb_x1000 * (int64_t)G_MPS2_X1000;
	if (tmp >= 0) {
		tmp += 500000;
	} else {
		tmp -= 500000;
	}
	return (int32_t)(tmp / 1000000);
}

static int32_t gyro_raw_to_rads_x1000(int16_t raw, uint32_t mdps_per_lsb)
{
	// gyro_rads_x1000 = (raw * mdps_per_lsb) * (pi/180)
	// Using Q1e9 for (pi/180): RAD_PER_DEG_Q1E9
	// result = raw * mdps_per_lsb * RAD_PER_DEG_Q1E9 / 1e9
	int64_t tmp = (int64_t)raw * (int64_t)mdps_per_lsb * (int64_t)RAD_PER_DEG_Q1E9;
	if (tmp >= 0) {
		tmp += 500000000;
	} else {
		tmp -= 500000000;
	}
	return (int32_t)(tmp / 1000000000);
}

void imu_service_init(void)
{
	s_next_sample_ms = HAL_GetTick();
	s_valid = false;
	for (int i = 0; i < 3; i++) {
		s_accel_mps2_x1000[i] = 0;
		s_gyro_rads_x1000[i] = 0;
	}
	s_last_update_ms = 0;

	// Configure IMU for stable telemetry sampling.
	// - ODR: 120 Hz (UI chain), sampled down to 50 Hz in this service
	// - FS: accel ±8g, gyro ±2000 dps
	const lsm6dsv16x_config_t cfg = {
		.i2c_addr = (lsm6dsv16x_i2c_addr_t)0x00u, // probe 0x6A/0x6B
		.odr_xl = LSM6DSV16X_ODR_120_HZ,
		.odr_g = LSM6DSV16X_ODR_120_HZ,
		.fs_xl = LSM6DSV16X_FS_XL_8G,
		.fs_g = LSM6DSV16X_FS_G_2000DPS,
	};

	(void)lsm6dsv16x_init(&cfg);
}

void imu_service_reset(void)
{
	imu_service_init();
}

void imu_service_tick(uint32_t now_ms)
{
	if ((int32_t)(now_ms - s_next_sample_ms) < 0) {
		return;
	}
	s_next_sample_ms = now_ms + IMU_SAMPLE_PERIOD_MS;

	int16_t a_raw[3];
	int16_t g_raw[3];
	if (!lsm6dsv16x_read_accel_gyro_raw(a_raw, g_raw)) {
		s_valid = false;
		return;
	}

	// Must match init config.
	const uint16_t a_mg_per_lsb_x1000 = accel_mg_per_lsb_x1000(LSM6DSV16X_FS_XL_8G);
	const uint32_t g_mdps_per_lsb = gyro_mdps_per_lsb(LSM6DSV16X_FS_G_2000DPS);

	for (int i = 0; i < 3; i++) {
		s_accel_mps2_x1000[i] = accel_raw_to_mps2_x1000(a_raw[i], a_mg_per_lsb_x1000);
		s_gyro_rads_x1000[i] = gyro_raw_to_rads_x1000(g_raw[i], g_mdps_per_lsb);
	}

	s_last_update_ms = now_ms;
	s_valid = true;
}

bool imu_service_get_accel_mps2_x1000(int32_t out_xyz[3])
{
	if ((out_xyz == NULL) || !s_valid) {
		return false;
	}
	for (int i = 0; i < 3; i++) {
		out_xyz[i] = s_accel_mps2_x1000[i];
	}
	return true;
}

bool imu_service_get_gyro_rads_x1000(int32_t out_xyz[3])
{
	if ((out_xyz == NULL) || !s_valid) {
		return false;
	}
	for (int i = 0; i < 3; i++) {
		out_xyz[i] = s_gyro_rads_x1000[i];
	}
	return true;
}

bool imu_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}
