#ifndef MLX90393_DRIVER_H
#define MLX90393_DRIVER_H

#include <stdint.h>
#include <stddef.h>

/* Defines from Adafruit Library */
#define MLX90393_DEFAULT_ADDR   (0x0C)
#define MLX90393_AXIS_ALL       (0x0E)

#define MLX90393_REG_SB         (0x10)
#define MLX90393_REG_SW         (0x20)
#define MLX90393_REG_SM         (0x30)
#define MLX90393_REG_RM         (0x40)
#define MLX90393_REG_RR         (0x50)
#define MLX90393_REG_WR         (0x60)
#define MLX90393_REG_EX         (0x80)
#define MLX90393_REG_RT         (0xF0)

#define MLX90393_CONF1          (0x00)
#define MLX90393_CONF2          (0x01)
#define MLX90393_CONF3          (0x02)

/* Enums */
typedef enum {
    MLX90393_GAIN_5X = 0,
    MLX90393_GAIN_4X,
    MLX90393_GAIN_3X,
    MLX90393_GAIN_2_5X,
    MLX90393_GAIN_2X,
    MLX90393_GAIN_1_67X,
    MLX90393_GAIN_1_33X,
    MLX90393_GAIN_1X
} mlx90393_gain_t;

typedef enum {
    MLX90393_RES_16 = 0,
    MLX90393_RES_17,
    MLX90393_RES_18,
    MLX90393_RES_19,
} mlx90393_resolution_t;

typedef enum {
    MLX90393_FILTER_0 = 0,
    MLX90393_FILTER_1,
    MLX90393_FILTER_2,
    MLX90393_FILTER_3,
    MLX90393_FILTER_4,
    MLX90393_FILTER_5,
    MLX90393_FILTER_6,
    MLX90393_FILTER_7,
} mlx90393_filter_t;

typedef enum {
    MLX90393_OSR_0 = 0,
    MLX90393_OSR_1,
    MLX90393_OSR_2,
    MLX90393_OSR_3,
} mlx90393_oversampling_t;

/* Interface Context */
typedef int32_t (*mlx90393_write_ptr)(void *, uint8_t *buf, uint16_t len);
typedef int32_t (*mlx90393_read_ptr)(void *, uint8_t *buf, uint16_t len);

typedef struct {
    mlx90393_write_ptr write; // Should write I2C bytes
    mlx90393_read_ptr read;   // Should read I2C bytes
    void *handle;
    
    // Config Cache
    mlx90393_gain_t gain;
    mlx90393_resolution_t res_x, res_y, res_z;
    mlx90393_filter_t dig_filt;
    mlx90393_oversampling_t osr;
} mlx90393_ctx_t;

/* Functions */
int32_t MLX90393_Init(mlx90393_ctx_t *ctx);
int32_t MLX90393_Reset(mlx90393_ctx_t *ctx);
int32_t MLX90393_SetGain(mlx90393_ctx_t *ctx, mlx90393_gain_t gain);
int32_t MLX90393_SetResolution(mlx90393_ctx_t *ctx, uint8_t axis, mlx90393_resolution_t res);
int32_t MLX90393_SetFilter(mlx90393_ctx_t *ctx, mlx90393_filter_t filter);
int32_t MLX90393_SetOversampling(mlx90393_ctx_t *ctx, mlx90393_oversampling_t osr);
int32_t MLX90393_StartMeasurement(mlx90393_ctx_t *ctx);
int32_t MLX90393_ReadMeasurement(mlx90393_ctx_t *ctx, float *x, float *y, float *z);

#endif
