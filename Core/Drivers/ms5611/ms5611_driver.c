#include "ms5611_driver.h"
#include "main.h"

// Helper to write command
static int32_t _send_cmd(ms5611_ctx_t *ctx, uint8_t cmd) {
    // Like SHT31, MS5611 commands are bare bytes.
    // Using write_reg(handle, cmd, NULL, 0) for command only?
    // Or correct: The cmd IS the register address effectively in many APIs?
    // Let's use write_reg(handle, cmd, NULL, 0) logic or similar.
    // Assuming abstraction supports 0 length data.
    return ctx->write_reg(ctx->handle, cmd, 0, 0);
}

static int32_t _read_prom(ms5611_ctx_t *ctx, uint8_t idx, uint16_t *val) {
    uint8_t buf[2];
    uint8_t cmd = MS5611_CMD_PROM_RD + (idx * 2);
    // Read 2 bytes from 'cmd' register
    if (ctx->read_reg(ctx->handle, cmd, buf, 2) != 0) return -1;
    *val = (buf[0] << 8) | buf[1];
    return 0;
}

static int32_t _read_adc(ms5611_ctx_t *ctx, uint32_t *val) {
    uint8_t buf[3];
    // Command 0x00 to read ADC result
    if (ctx->read_reg(ctx->handle, MS5611_CMD_ADC_READ, buf, 3) != 0) return -1;
    *val = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    return 0;
}

int32_t MS5611_Init(ms5611_ctx_t *ctx) {
    if (!ctx->address) ctx->address = MS5611_I2C_ADDR_HIGH; // Default
    
    // Reset
    _send_cmd(ctx, MS5611_CMD_RESET);
    // Need Delay ~3ms
    
    // Read PROM C1-C6
    for (int i=1; i<=6; i++) {
        if (_read_prom(ctx, i, &ctx->C[i]) != 0) return -1;
    }
    return 0;
}

int32_t MS5611_Read_PT(ms5611_ctx_t *ctx, int32_t *press_pa, int32_t *temp_c_x100) {
    // 1. Convert D1 (Pressure)
    uint8_t cmd_d1 = MS5611_CMD_CONV_D1 | MS5611_OSR_4096;
    _send_cmd(ctx, cmd_d1);
    // Delay ~9ms
#ifndef UNIT_TEST
    HAL_Delay(10);
#endif
    
    uint32_t D1 = 0;
    _read_adc(ctx, &D1);
    
    // 2. Convert D2 (Temp)
    uint8_t cmd_d2 = MS5611_CMD_CONV_D2 | MS5611_OSR_4096;
    _send_cmd(ctx, cmd_d2);
    // Delay ~9ms
#ifndef UNIT_TEST
    HAL_Delay(10);
#endif
    
    uint32_t D2 = 0;
    _read_adc(ctx, &D2);
    
    // 3. Calculate
    // D2 is raw temp, D1 is raw pressure
    // dT = D2 - C5 * 2^8
    int64_t dT = (int64_t)D2 - ((int64_t)ctx->C[5] << 8);
    
    // TEMP = 2000 + dT * C6 / 2^23
    int64_t TEMP = 2000 + (dT * (int64_t)ctx->C[6] >> 23);
    
    // OFF = C2 * 2^16 + (C4 * dT) / 2^7
    int64_t OFF = ((int64_t)ctx->C[2] << 16) + (( (int64_t)ctx->C[4] * dT ) >> 7);
    
    // SENS = C1 * 2^15 + (C3 * dT) / 2^8
    int64_t SENS = ((int64_t)ctx->C[1] << 15) + (( (int64_t)ctx->C[3] * dT ) >> 8);
    
    // P = (D1 * SENS / 2^21 - OFF) / 2^15
    int64_t P = (((D1 * SENS) >> 21) - OFF) >> 15;
    
    *press_pa = (int32_t)P;
    *temp_c_x100 = (int32_t)TEMP;
    
    return 0;
}
