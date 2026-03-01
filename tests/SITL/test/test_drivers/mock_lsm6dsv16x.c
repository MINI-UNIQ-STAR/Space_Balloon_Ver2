#include "lsm6dsv16x_reg.h"
// #include <stdio.h>

/** 
 * @file mock_lsm6dsv16x.c
 * @brief 테스트 드라이버를 위한 LSM6DSV16X Mock 구현
 * @details 실제 하드웨어 없이 드라이버 함수 호출을 받아주는 Stub 함수들입니다.
 */

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
    return ((float)lsb) * 0.061f; /**< 일반적인 감도 값 적용 */
}

float lsm6dsv16x_from_fs2000_to_mdps(int16_t lsb) {
    return ((float)lsb) * 70.0f; /**< 일반적인 감도 값 적용 */
}
