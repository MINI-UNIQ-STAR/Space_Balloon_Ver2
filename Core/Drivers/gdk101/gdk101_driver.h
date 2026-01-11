#ifndef GDK101_DRIVER_H
#define GDK101_DRIVER_H

// Error Codes
#define GDK101_OK             0
#define GDK101_I2C_ERR       -1
#define GDK101_ID_ERR        -2
#define GDK101_RESET_ERR     -3

#include <stdint.h>
#include <stdbool.h>

#define GDK101_I2C_ADDR       0x18 // Default 7-bit address? Library says explicit ctor(addr). 
                                   // Manual says 0x18 usually. 
                                   // NOTE: Library uses 8-bit addressing logic? 
                                   // Let's assume standard 7-bit addr passed by user.

// Registers
#define GDK101_REG_RESET      0xA0
#define GDK101_REG_STATUS     0xB0
#define GDK101_REG_TIME       0xB1
#define GDK101_REG_AVG_10MIN  0xB2
#define GDK101_REG_AVG_1MIN   0xB3
#define GDK101_REG_FW_VER     0xB4

// Constant
#define SV_TO_RTG_CONST       107.185f

typedef int32_t (*gdk101_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*gdk101_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

typedef struct {
    gdk101_write_ptr write_reg;
    gdk101_read_ptr read_reg;
    void *handle;
    uint8_t address;
} gdk101_ctx_t;

int32_t GDK101_Init(gdk101_ctx_t *ctx);
int32_t GDK101_Reset(gdk101_ctx_t *ctx);
int32_t GDK101_Read_10Min_Avg(gdk101_ctx_t *ctx, float *uSv_h);
int32_t GDK101_Read_1Min_Avg(gdk101_ctx_t *ctx, float *uSv_h);
int32_t GDK101_Read_Status(gdk101_ctx_t *ctx, uint8_t *status, bool *vibration);

#endif
