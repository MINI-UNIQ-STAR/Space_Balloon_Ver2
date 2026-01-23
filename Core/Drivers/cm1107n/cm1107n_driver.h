#ifndef CM1107N_DRIVER_H
#define CM1107N_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file cm1107n_driver.h
 * @brief Cubic CM1107N CO2 Sensor Driver
 * 
 * @details Protocol: UART command format over I2C bus
 *          - I2C Address: 0x31 (7-bit)
 *          - Send: 0x11 0x01 0x01 0xED (Read CO2 command)
 *          - Recv: 0x16 0x05 0x01 [DF1] [DF2] [DF3] [DF4] [CS]
 *          - CO2 (ppm) = DF1*256 + DF2
 *          - DF3 = Status (0x01=OK, 0x00=init, 0x02=error)
 *          - CS = (256 - sum(bytes 0-6)) % 256
 * 
 * @note This driver uses UART protocol encapsulated in I2C transactions.
 *       The CM1107N supports both native UART and I2C interfaces.
 *       This implementation chose UART-over-I2C for consistency.
 * 
 * @ref Cubic CM1107N Datasheet - UART Communication Protocol
 */

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
