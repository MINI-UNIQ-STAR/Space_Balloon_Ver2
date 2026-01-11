#include "cm1107n_driver.h"
#include "main.h" // For HAL_Delay

int32_t CM1107N_Init(cm1107n_ctx_t *ctx) {
    if (!ctx->address) ctx->address = CM1107N_I2C_ADDR;
    ctx->co2_ppm = 0;
    return 0;
}

int32_t CM1107N_ReadCO2(cm1107n_ctx_t *ctx, uint16_t *co2_ppm) {
    uint8_t cmd[] = {0x11, 0x01, 0x01, 0xED};
    uint8_t resp[8];
    
    // I2C Write: Send 4-byte command. 
    // We treat the first byte (0x11) as "register" or just payload.
    // Standard I2C Write: [Addr][Reg/Data]...
    // Let's use write_reg with cmd[0] as reg, and cmd[1..3] as data.
    if (ctx->write(ctx->handle, cmd[0], &cmd[1], 3) != 0) return -1;
    
    // Wait for sensor processing (Datasheet requirement ~20ms)
    HAL_Delay(20);
    
    // I2C Read: Read 8 bytes.
    // We pass 0 as register/dummy.
    if (ctx->read(ctx->handle, 0, resp, 8) != 0) return -1;
    
    // Verify Header and Length
    // Resp: 16 05 01 [DF1] [DF2] [DF3] [DF4] [CS]
    if (resp[0] != 0x16 || resp[1] != 0x05 || resp[2] != 0x01) return -2;
    
    // Checksum Check
    uint16_t sum = 0;
    for (int i=0; i<7; i++) sum += resp[i];
    uint8_t cs = (256 - (sum % 256)) % 256;
    if (cs != resp[7]) return -3;
    
    // Extract CO2
    *co2_ppm = (resp[3] << 8) | resp[4];
    ctx->co2_ppm = *co2_ppm;
    
    return 0;
}
