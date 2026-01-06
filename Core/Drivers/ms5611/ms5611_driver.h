#ifndef MS5611_DRIVER_H
#define MS5611_DRIVER_H

#include <stdint.h>

#define MS5611_I2C_ADDR_HIGH    0x76
#define MS5611_I2C_ADDR_LOW     0x77

#define MS5611_CMD_RESET        0x1E
#define MS5611_CMD_ADC_READ     0x00
#define MS5611_CMD_PROM_RD      0xA0
#define MS5611_CMD_CONV_D1      0x40 // + OSR offset
#define MS5611_CMD_CONV_D2      0x50 // + OSR offset

#define MS5611_OSR_4096         0x08

typedef int32_t (*ms5611_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*ms5611_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

typedef struct {
    ms5611_write_ptr write_reg;
    ms5611_read_ptr read_reg;
    void *handle;
    uint8_t address;
    
    // Calibration Data
    uint16_t C[7]; // C[1]..C[6] used
} ms5611_ctx_t;

int32_t MS5611_Init(ms5611_ctx_t *ctx);
int32_t MS5611_Read_PT(ms5611_ctx_t *ctx, int32_t *press_pa, int32_t *temp_c_x100);

#endif
