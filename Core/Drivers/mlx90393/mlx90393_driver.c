#include "mlx90393_driver.h"
#include <string.h>

// Lookup tables from Adafruit Library (HallConf=0xC default)
static const float mlx90393_lsb_lookup[8][4][2] = {
    {{0.751, 1.210}, {1.502, 2.420}, {3.004, 4.840}, {6.009, 9.680}}, // 5x
    {{0.601, 0.968}, {1.202, 1.936}, {2.403, 3.872}, {4.840, 7.744}}, // 4x
    {{0.451, 0.726}, {0.901, 1.452}, {1.803, 2.904}, {3.605, 5.808}}, // 3x
    {{0.376, 0.605}, {0.751, 1.210}, {1.502, 2.420}, {3.004, 4.840}}, // 2.5x
    {{0.300, 0.484}, {0.601, 0.968}, {1.202, 1.936}, {2.403, 3.872}}, // 2x
    {{0.250, 0.403}, {0.501, 0.807}, {1.001, 1.613}, {2.003, 3.227}}, // 1.6x
    {{0.200, 0.323}, {0.401, 0.645}, {0.801, 1.291}, {1.602, 2.581}}, // 1.3x
    {{0.150, 0.242}, {0.300, 0.484}, {0.601, 0.968}, {1.202, 1.936}}  // 1x
};



static int32_t MLX90393_WriteReg(mlx90393_ctx_t *ctx, uint8_t reg, uint16_t data) {
    uint8_t tx[4] = {
        MLX90393_REG_WR,
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF),
        (uint8_t)(reg << 2)
    };
    uint8_t status;
    // Read 1 byte (status) implicitly by transceive if logic above was full transceive
    // But typically Write command returns 1 status byte
    // Adafruit transceive does Write -> Delay -> Read(len+1)
    
    // Let's refine Transceive:
    // It should Read (rx_len + 1). 
    // If rx_len is 0, it still reads 1 byte status? Adafruit implementation does.
    
    // Simpler:
    if (ctx->write(ctx->handle, tx, 4) != 0) return -1;
    // Read status
    uint8_t stat;
    if (ctx->read(ctx->handle, &stat, 1) != 0) return -1;
    return 0;
}

static int32_t MLX90393_ReadReg(mlx90393_ctx_t *ctx, uint8_t reg, uint16_t *data) {
    uint8_t tx[2] = { MLX90393_REG_RR, (uint8_t)(reg << 2) };
    if (ctx->write(ctx->handle, tx, 2) != 0) return -1;
    
    uint8_t rx[3]; // Status, High, Low
    if (ctx->read(ctx->handle, rx, 3) != 0) return -1;
    
    *data = ((uint16_t)rx[1] << 8) | rx[2];
    return 0;
}

int32_t MLX90393_Init(mlx90393_ctx_t *ctx) {
    // Exit Mode
    uint8_t tx = MLX90393_REG_EX;
    if (ctx->write(ctx->handle, &tx, 1) != 0) return -1;
    uint8_t stat;
    if (ctx->read(ctx->handle, &stat, 1) != 0) return -1;
    
    // Reset
    if (MLX90393_Reset(ctx) != 0) return -1;
    
    // Defaults matching Adafruit
    MLX90393_SetGain(ctx, MLX90393_GAIN_1X);
    MLX90393_SetResolution(ctx, 0, MLX90393_RES_16);
    MLX90393_SetResolution(ctx, 1, MLX90393_RES_16);
    MLX90393_SetResolution(ctx, 2, MLX90393_RES_16);
    MLX90393_SetOversampling(ctx, MLX90393_OSR_3);
    MLX90393_SetFilter(ctx, MLX90393_FILTER_7);
    
    return 0;
}

int32_t MLX90393_Reset(mlx90393_ctx_t *ctx) {
    uint8_t tx = MLX90393_REG_RT;
    if (ctx->write(ctx->handle, &tx, 1) != 0) return -1;
    uint8_t stat;
    if (ctx->read(ctx->handle, &stat, 1) != 0) return -1; // Should be Reset Status
    return 0;
}

int32_t MLX90393_SetGain(mlx90393_ctx_t *ctx, mlx90393_gain_t gain) {
    ctx->gain = gain;
    uint16_t data;
    if (MLX90393_ReadReg(ctx, MLX90393_CONF1, &data) != 0) return -1;
    data &= ~0x0070;
    data |= (gain << 4);
    return MLX90393_WriteReg(ctx, MLX90393_CONF1, data);
}

int32_t MLX90393_SetResolution(mlx90393_ctx_t *ctx, uint8_t axis, mlx90393_resolution_t res) {
    uint16_t data;
    if (MLX90393_ReadReg(ctx, MLX90393_CONF3, &data) != 0) return -1;
    
    switch(axis) {
        case 0: // X
            ctx->res_x = res;
            data &= ~0x0060;
            data |= (res << 5);
            break;
        case 1: // Y
            ctx->res_y = res;
            data &= ~0x0180;
            data |= (res << 7);
            break;
        case 2: // Z
            ctx->res_z = res;
            data &= ~0x0600;
            data |= (res << 9);
            break;
    }
    return MLX90393_WriteReg(ctx, MLX90393_CONF3, data);
}

int32_t MLX90393_SetFilter(mlx90393_ctx_t *ctx, mlx90393_filter_t filter) {
    ctx->dig_filt = filter;
    uint16_t data;
    if (MLX90393_ReadReg(ctx, MLX90393_CONF3, &data) != 0) return -1;
    data &= ~0x1C;
    data |= (filter << 2);
    return MLX90393_WriteReg(ctx, MLX90393_CONF3, data);
}

int32_t MLX90393_SetOversampling(mlx90393_ctx_t *ctx, mlx90393_oversampling_t osr) {
    ctx->osr = osr;
    uint16_t data;
    if (MLX90393_ReadReg(ctx, MLX90393_CONF3, &data) != 0) return -1;
    data &= ~0x03;
    data |= osr;
    return MLX90393_WriteReg(ctx, MLX90393_CONF3, data);
}

int32_t MLX90393_StartMeasurement(mlx90393_ctx_t *ctx) {
    uint8_t tx = MLX90393_REG_SM | MLX90393_AXIS_ALL;
    if (ctx->write(ctx->handle, &tx, 1) != 0) return -1;
    uint8_t stat;
    if (ctx->read(ctx->handle, &stat, 1) != 0) return -1;
    return 0;
}

int32_t MLX90393_ReadMeasurement(mlx90393_ctx_t *ctx, float *x, float *y, float *z) {
    uint8_t tx = MLX90393_REG_RM | MLX90393_AXIS_ALL;
    if (ctx->write(ctx->handle, &tx, 1) != 0) return -1;
    
    // Read Status + 6 bytes data
    uint8_t rx[7];
    if (ctx->read(ctx->handle, rx, 7) != 0) return -1;
    
    int16_t xi = (rx[1] << 8) | rx[2];
    int16_t yi = (rx[3] << 8) | rx[4];
    int16_t zi = (rx[5] << 8) | rx[6];
    
    // Apply conversion
    // Assuming gain/res cached correctly in ctx
    // Using lookup table [gain][res][0/1] where 0 is xy, 1 is z
    
    // Note: Lookup table index: [gain][res][0 or 1]
    // gain index: ctx->gain
    // res index: ctx->res_x (assuming same res used or lookup per axis)
    
    *x = (float)xi * mlx90393_lsb_lookup[ctx->gain][ctx->res_x][0];
    *y = (float)yi * mlx90393_lsb_lookup[ctx->gain][ctx->res_y][0];
    *z = (float)zi * mlx90393_lsb_lookup[ctx->gain][ctx->res_z][1];
    
    return 0;
}
