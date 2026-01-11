#include "lsm6dsv16x_reg.h"
// #include <stdio.h>

// Mock Implementation for Test Drivers

int32_t lsm6dsv16x_device_id_get(const stmdev_ctx_t *ctx, uint8_t *val) {
    *val = LSM6DSV16X_ID;
    return 0;
}

int32_t lsm6dsv16x_sw_reset(const stmdev_ctx_t *ctx) {
    uint8_t data = 1; 
    return ctx->write_reg(ctx->handle, LSM6DSV16X_CTRL3, &data, 1);
}

int32_t lsm6dsv16x_xl_data_rate_set(const stmdev_ctx_t *ctx, lsm6dsv16x_data_rate_t val) {
    return 0;
}

int32_t lsm6dsv16x_gy_data_rate_set(const stmdev_ctx_t *ctx, lsm6dsv16x_data_rate_t val) {
     return 0;
}

int32_t lsm6dsv16x_xl_mode_set(const stmdev_ctx_t *ctx, lsm6dsv16x_xl_mode_t val) {
     return 0;
}

int32_t lsm6dsv16x_gy_mode_set(const stmdev_ctx_t *ctx, lsm6dsv16x_gy_mode_t val) {
     return 0;
}

int32_t lsm6dsv16x_sflp_game_rotation_set(const stmdev_ctx_t *ctx, uint8_t val) {
     return 0;
}

int32_t lsm6dsv16x_sflp_data_rate_set(const stmdev_ctx_t *ctx, lsm6dsv16x_sflp_data_rate_t val) {
     return 0;
}

int32_t lsm6dsv16x_acceleration_raw_get(const stmdev_ctx_t *ctx, int16_t *val) {
     val[0] = 0; val[1] = 0; val[2] = 0;
     return 0;
}

int32_t lsm6dsv16x_angular_rate_raw_get(const stmdev_ctx_t *ctx, int16_t *val) {
     val[0] = 0; val[1] = 0; val[2] = 0;
     return 0;
}

float lsm6dsv16x_from_fs2_to_mg(int16_t lsb) {
    return ((float)lsb) * 0.061f; // Typical sensitivity
}

float lsm6dsv16x_from_fs2000_to_mdps(int16_t lsb) {
    return ((float)lsb) * 70.0f; // Typical sensitivity
}
