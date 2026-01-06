#include "sen0321_driver.h"

int32_t SEN0321_Init(sen0321_ctx_t *ctx) {
    if (!ctx->address) ctx->address = SEN0321_I2C_ADDR_0;
    
    // Set to Automatic Mode
    uint8_t mode = SEN0321_MODE_AUTO;
    // Note: DFRobot uses Write(reg, val) which is basically MemWrite
    return ctx->write_reg(ctx->handle, SEN0321_REG_MODE, &mode, 1);
}

int32_t SEN0321_ReadOzone(sen0321_ctx_t *ctx, int16_t *ozone_ppb) {
    uint8_t buf[2];
    // Read 2 bytes from AUTO_DATA_H (0x09)
    // Assuming Automatic Mode is active and updates continuously
    if (ctx->read_reg(ctx->handle, SEN0321_REG_AUTO_DATA_H, buf, 2) != 0) return -1;
    
    *ozone_ppb = ((int16_t)buf[0] << 8) | buf[1];
    return 0;
}
