#ifndef SEN0321_DRIVER_H
#define SEN0321_DRIVER_H

#include <stdint.h>

#define SEN0321_I2C_ADDR_0          0x70
#define SEN0321_I2C_ADDR_1          0x71
#define SEN0321_I2C_ADDR_2          0x72
#define SEN0321_I2C_ADDR_3          0x73

#define SEN0321_MODE_AUTO           0x00
#define SEN0321_MODE_PASSIVE        0x01

#define SEN0321_REG_MODE            0x03
#define SEN0321_REG_SET_PASSIVE     0x04
#define SEN0321_REG_AUTO_DATA_H     0x09
#define SEN0321_REG_AUTO_DATA_L     0x0A
#define SEN0321_REG_PASS_DATA_H     0x07
#define SEN0321_REG_PASS_DATA_L     0x08

typedef int32_t (*sen0321_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*sen0321_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

typedef struct {
    sen0321_write_ptr write_reg;
    sen0321_read_ptr read_reg; // Standard Register Read
    void *handle;
    uint8_t address; // 7-bit address
} sen0321_ctx_t;

int32_t SEN0321_Init(sen0321_ctx_t *ctx);
int32_t SEN0321_ReadOzone(sen0321_ctx_t *ctx, int16_t *ozone_ppb);

#endif
