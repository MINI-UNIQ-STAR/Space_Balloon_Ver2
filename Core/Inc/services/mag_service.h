#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Magnetometer Service (MLX90393)

void mag_service_init(void);
void mag_service_tick(uint32_t now_ms);

bool mag_service_get_data(float *x, float *y, float *z);

#ifdef __cplusplus
}
#endif
