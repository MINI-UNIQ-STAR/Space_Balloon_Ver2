#include "ms5611_driver.h"
#include "main.h"
#include <stdio.h>

// Helper to write command
static int32_t _send_cmd(ms5611_ctx_t *ctx, uint8_t cmd) {
    // Like SHT31, MS5611 commands are bare bytes.
    // Using write_reg(handle, cmd, NULL, 0) for command only?
    // Or correct: The cmd IS the register address effectively in many APIs?
    // Let's use write_reg(handle, cmd, NULL, 0) logic or similar.
    // Assuming abstraction supports 0 length data.
    return ctx->write_reg(ctx->handle, cmd, 0, 0);
}

static int32_t _read_prom(ms5611_ctx_t *ctx, uint8_t idx, uint16_t *val) {
    uint8_t buf[2];
    uint8_t cmd = MS5611_CMD_PROM_RD + (idx * 2);
    // Read 2 bytes from 'cmd' register
    if (ctx->read_reg(ctx->handle, cmd, buf, 2) != 0) return -1;
    *val = (buf[0] << 8) | buf[1];
    return 0;
}

static int32_t _read_adc(ms5611_ctx_t *ctx, uint32_t *val) {
    uint8_t buf[3];
    // Command 0x00 to read ADC result
    if (ctx->read_reg(ctx->handle, MS5611_CMD_ADC_READ, buf, 3) != 0) return -1;
    *val = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    return 0;
}

// State definitions
#define S_IDLE_START_D1 0
#define S_WAIT_D1       1
#define S_START_D2      2
#define S_WAIT_D2       3

int32_t MS5611_Init(ms5611_ctx_t *ctx) {
    if (!ctx->address) ctx->address = MS5611_I2C_ADDR_HIGH; // Default
    
    // Reset state
    ctx->state = S_IDLE_START_D1;
    ctx->tick_start = 0;

    // Reset Command
    _send_cmd(ctx, MS5611_CMD_RESET);
    // Need Delay ~3ms - In init phase, blocking is acceptable or we should assume caller waits.
    // For now, keeping blocking in Init as Init is usually done once at startup.
    #ifndef UNIT_TEST
    HAL_Delay(5);
    #endif
    
    // Read PROM C1-C6 (indices 1-6)
    // Also read C0 (Factory data) and C7 (CRC) for validation
    uint16_t prom[8];
    prom[0] = 0; // C0 - factory reserved
    for (int i=0; i<=7; i++) {
        if (_read_prom(ctx, i, &prom[i]) != 0) return -1;
    }
    
    // Copy C1-C6 to context
    for (int i=1; i<=6; i++) {
        ctx->C[i] = prom[i];
    }
    
    // CRC4 Verification (FMEA S-07 mitigation)
    uint16_t crc_read = prom[7] & 0x000F; // Last 4 bits of PROM[7]
    prom[7] = (prom[7] & 0xFF00); // CRC byte removed for calculation
    
    uint16_t n_rem = 0;
    for (int cnt = 0; cnt < 16; cnt++) {
        if (cnt % 2 == 1) {
            n_rem ^= (prom[cnt >> 1] & 0x00FF);
        } else {
            n_rem ^= (prom[cnt >> 1] >> 8);
        }
        for (int n_bit = 8; n_bit > 0; n_bit--) {
            if (n_rem & 0x8000) {
                n_rem = (n_rem << 1) ^ 0x3000;
            } else {
                n_rem = (n_rem << 1);
            }
        }
    }
    n_rem = (n_rem >> 12) & 0x000F;
    
    if (n_rem != crc_read) {
        #ifdef DEBUG
        printf("MS5611: PROM CRC4 mismatch! calc=%u read=%u\n", n_rem, crc_read);
        #endif
        return -2; // CRC Error
    }
    
    return 0;
}

int32_t MS5611_Read_PT(ms5611_ctx_t *ctx, int32_t *press_pa, int32_t *temp_c_x100) {
    uint32_t now = HAL_GetTick();

    switch (ctx->state) {
        case S_IDLE_START_D1:
        {
            // 1. Send Command Convert D1 (Pressure)
            uint8_t cmd_d1 = MS5611_CMD_CONV_D1 | MS5611_OSR_4096;
            if (_send_cmd(ctx, cmd_d1) != 0) return MS5611_ERROR;
            
            ctx->tick_start = now;
            ctx->state = S_WAIT_D1;
            return MS5611_BUSY;
        }

        case S_WAIT_D1:
        {
            // Check 10ms delay
            if ((now - ctx->tick_start) < 10) return MS5611_BUSY;

            // Read D1
            if (_read_adc(ctx, &ctx->D1_raw) != 0) {
                ctx->state = S_IDLE_START_D1; // Retry next time
                return MS5611_ERROR;
            }

            // Immediately Start D2
            ctx->state = S_START_D2;
            // Fallthrough to S_START_D2 to save one cycle? 
            // Better to return busy to keep it simple or execute immediately?
            // Let's execute immediately to start D2 conversion right away.
        }
        /* Fallthrough */

        case S_START_D2:
        {
             // 2. Send Command Convert D2 (Temp)
            uint8_t cmd_d2 = MS5611_CMD_CONV_D2 | MS5611_OSR_4096;
            if (_send_cmd(ctx, cmd_d2) != 0) {
                 ctx->state = S_IDLE_START_D1;
                 return MS5611_ERROR;
            }

            ctx->tick_start = now; // Update timestamp for D2 wait
            ctx->state = S_WAIT_D2;
            return MS5611_BUSY;
        }

        case S_WAIT_D2:
        {
            // Check 10ms delay
            if ((now - ctx->tick_start) < 10) return MS5611_BUSY;

            // Read D2
            if (_read_adc(ctx, &ctx->D2_raw) != 0) {
                ctx->state = S_IDLE_START_D1;
                return MS5611_ERROR;
            }

            // 3. Calculate
            // D2 is raw temp, D1 is raw pressure
            // dT = D2 - C5 * 2^8
            int64_t dT = (int64_t)ctx->D2_raw - ((int64_t)ctx->C[5] << 8);
            
            // TEMP = 2000 + dT * C6 / 2^23
            int64_t TEMP = 2000 + (dT * (int64_t)ctx->C[6] >> 23);
            
            // OFF = C2 * 2^16 + (C4 * dT) / 2^7
            int64_t OFF = ((int64_t)ctx->C[2] << 16) + (( (int64_t)ctx->C[4] * dT ) >> 7);
            
            // SENS = C1 * 2^15 + (C3 * dT) / 2^8
            int64_t SENS = ((int64_t)ctx->C[1] << 15) + (( (int64_t)ctx->C[3] * dT ) >> 8);
            
            // P = (D1 * SENS / 2^21 - OFF) / 2^15
            int64_t P = (((ctx->D1_raw * SENS) >> 21) - OFF) >> 15;
            
            *press_pa = (int32_t)P;
            *temp_c_x100 = (int32_t)TEMP;
            
            // Reset to start for next reading
            ctx->state = S_IDLE_START_D1;
            return MS5611_OK; // Data Ready
        }

        default:
            ctx->state = S_IDLE_START_D1;
            return MS5611_ERROR;
    }
}
