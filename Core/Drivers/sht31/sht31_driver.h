#ifndef SHT31_DRIVER_H
#define SHT31_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define SHT31_I2C_ADDR_DEFAULT  0x44
#define SHT31_I2C_ADDR_ALT      0x45

#define SHT31_MEAS_HIGHREP_STRETCH 0x2C06
#define SHT31_MEAS_HIGHREP         0x2400
#define SHT31_READSTATUS           0xF32D
#define SHT31_SOFTRESET            0x30A2
#define SHT31_HEATEREN             0x306D
#define SHT31_HEATERDIS            0x3066

typedef int32_t (*sht31_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*sht31_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

typedef struct {
    sht31_write_ptr write_reg;
    sht31_read_ptr read_reg;
    void *handle; // generic handle for I2C instance
    uint8_t address;
} sht31_ctx_t;

int32_t SHT31_Init(sht31_ctx_t *ctx);
int32_t SHT31_Reset(sht31_ctx_t *ctx);
int32_t SHT31_ReadTempHum(sht31_ctx_t *ctx, float *temp_c, float *rh);
int32_t SHT31_SetHeater(sht31_ctx_t *ctx, bool enable);


#endif
