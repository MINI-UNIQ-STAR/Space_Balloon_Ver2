#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// MLX90393 Magnetometer (I2C1)
// Default I2C Address (A0=0, A1=0) -> 0x0C
#define MLX90393_I2C_ADDR_DEFAULT (0x0C << 1)

typedef struct {
    uint8_t i2c_addr;
} mlx90393_config_t;

// Initializes the device.
bool mlx90393_init(const mlx90393_config_t *cfg);

// Reads the latest data (X, Y, Z in uT).
bool mlx90393_read_data(float *x, float *y, float *z);

// Callback for EXTI interrupt (Data Ready).
void mlx90393_exti_callback(uint16_t pin);

#ifdef __cplusplus
}
#endif
