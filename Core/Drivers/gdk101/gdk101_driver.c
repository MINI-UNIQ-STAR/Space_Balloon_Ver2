#include "gdk101_driver.h"

// Note on I2C Read for GDK101: 
// Library implementation:
// 1. Write (addr, reg)
// 2. Delay(10)
// 3. Request(addr, 2 bytes)
// 4. Read 2 bytes

int32_t GDK101_Init(gdk101_ctx_t *ctx) {
    // Just read status to check connection
    uint8_t status;
    bool vib;
    return GDK101_Read_Status(ctx, &status, &vib);
}

int32_t GDK101_Reset(gdk101_ctx_t *ctx) {
    // Write Reset command (0xA0)
    // The library does a read after reset?? 
    // gamma_mod_read(RESET) -> writes 0xA0, then reads 2 bytes.
    // Let's mimic library behavior.
    uint8_t buf[2];
    // Write 0xA0
    // We can assume read_reg handles the Write-Restart-Read sequence
    if (ctx->read_reg(ctx->handle, GDK101_REG_RESET, buf, 2) != 0) return -1;
    return 0;
}

int32_t GDK101_Read_10Min_Avg(gdk101_ctx_t *ctx, float *uSv_h) {
    uint8_t buf[2];
    if (ctx->read_reg(ctx->handle, GDK101_REG_AVG_10MIN, buf, 2) != 0) return -1;
    
    // Value = buf[0] + buf[1] / 100.0
    *uSv_h = buf[0] + (float)buf[1] / 100.0f;
    return 0;
}

int32_t GDK101_Read_1Min_Avg(gdk101_ctx_t *ctx, float *uSv_h) {
    uint8_t buf[2];
    if (ctx->read_reg(ctx->handle, GDK101_REG_AVG_1MIN, buf, 2) != 0) return -1;
    
    *uSv_h = buf[0] + (float)buf[1] / 100.0f;
    return 0;
}

int32_t GDK101_Read_Status(gdk101_ctx_t *ctx, uint8_t *status, bool *vibration) {
    uint8_t buf[2];
    if (ctx->read_reg(ctx->handle, GDK101_REG_STATUS, buf, 2) != 0) return -1;
    
    *status = buf[0];
    *vibration = buf[1];
    return 0;
}
