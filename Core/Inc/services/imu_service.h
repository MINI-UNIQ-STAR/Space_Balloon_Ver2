#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// IMU service (LSM6DSV16x on I2C1)

void imu_service_init(void);
void imu_service_reset(void);

// Non-blocking periodic sampling (stores most recent accel+gyro).
void imu_service_tick(uint32_t now_ms);

// Latest accel in m/s^2 * 1000 (XYZ).
bool imu_service_get_accel_mps2_x1000(int32_t out_xyz[3]);

// Latest gyro in rad/s * 1000 (XYZ).
bool imu_service_get_gyro_rads_x1000(int32_t out_xyz[3]);

// Latest attitude (Roll, Pitch) in degrees.
bool imu_service_get_attitude(float *out_roll_deg, float *out_pitch_deg);

bool imu_service_get_last_update_ms(uint32_t *out_ms);

// Latest gyro in rad/s * 1000 (XYZ).
bool imu_service_get_gyro_rads_x1000(int32_t out_xyz[3]);

bool imu_service_get_last_update_ms(uint32_t *out_ms);

#ifdef __cplusplus
}
#endif
