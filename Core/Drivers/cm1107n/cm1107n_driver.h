#ifndef CM1107N_DRIVER_H
#define CM1107N_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// CM1107N UART Protocol
// Send: 11 01 01 ED
// Recv: 16 05 01 [DF1] [DF2] [DF3] [DF4] [CS]
// CO2 (ppm) = DF1*256 + DF2

typedef int32_t (*cm1107n_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*cm1107n_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

#define CM1107N_I2C_ADDR 0x31

typedef struct {
    cm1107n_write_ptr write;
    cm1107n_read_ptr read;
    void *handle;
    uint8_t address; // Added for I2C
    
    uint16_t co2_ppm;
} cm1107n_ctx_t;

int32_t CM1107N_Init(cm1107n_ctx_t *ctx);
int32_t CM1107N_ReadCO2(cm1107n_ctx_t *ctx, uint16_t *co2_ppm);

#endif
