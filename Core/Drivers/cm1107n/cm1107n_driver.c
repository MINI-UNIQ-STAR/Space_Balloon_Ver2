#include "cm1107n_driver.h"

int32_t CM1107N_Init(cm1107n_ctx_t *ctx) {
    ctx->co2_ppm = 0;
    return 0;
}

int32_t CM1107N_ReadCO2(cm1107n_ctx_t *ctx, uint16_t *co2_ppm) {
    uint8_t cmd[] = {0x11, 0x01, 0x01, 0xED};
    uint8_t resp[8];
    
    // Send Command
    if (ctx->write(ctx->handle, cmd, 4) != 0) return -1;
    
    // Read Response (Blocking for now, or assume buffer filled)
    if (ctx->read(ctx->handle, resp, 8) != 0) return -1;
    
    // Verify Header and Length
    if (resp[0] != 0x16 || resp[1] != 0x05 || resp[2] != 0x01) return -2;
    
    // Checksum Check: 256 - (Sum of bytes 0 to 6) % 256 = Byte 7
    uint16_t sum = 0;
    for (int i=0; i<7; i++) sum += resp[i];
    uint8_t cs = (256 - (sum % 256)) % 256;
    if (cs != resp[7]) return -3;
    
    // Extract CO2
    *co2_ppm = (resp[3] << 8) | resp[4];
    ctx->co2_ppm = *co2_ppm;
    
    return 0;
}
