#include "services/mag_service.h"
#include "drivers/mlx90393.h"
#include <stddef.h>

static float s_mag_x, s_mag_y, s_mag_z;
static bool s_valid = false;
static uint32_t s_last_read_ms = 0;

void mag_service_init(void) {
    mlx90393_config_t cfg = { .i2c_addr = MLX90393_I2C_ADDR_DEFAULT };
    mlx90393_init(&cfg);
    s_valid = false;
}

void mag_service_tick(uint32_t now_ms) {
    // Poll every 100ms (10Hz)
    if (now_ms - s_last_read_ms < 100) {
        return;
    }

    if (mlx90393_read_data(&s_mag_x, &s_mag_y, &s_mag_z)) {
        s_valid = true;
        s_last_read_ms = now_ms;
    }
}

bool mag_service_get_data(float *x, float *y, float *z) {
    if (!s_valid) return false;
    if (x) *x = s_mag_x;
    if (y) *y = s_mag_y;
    if (z) *z = s_mag_z;
    return true;
}

bool mag_service_get_last_update_ms(uint32_t *out_ms) {
    if (!s_valid || out_ms == NULL) return false;
    *out_ms = s_last_read_ms;
    return true;
}

void mag_service_reset(void) {
    s_valid = false;
    s_last_read_ms = 0;
    mlx90393_config_t cfg = { .i2c_addr = MLX90393_I2C_ADDR_DEFAULT };
    mlx90393_init(&cfg);
}
