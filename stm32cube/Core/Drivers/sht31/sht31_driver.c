#include "sht31_driver.h"
#include <string.h> // for NULL check if needed
#include "main.h" // for HAL_GetTick

/* 
 * SHT31 Command Write Logic:
 * The sensor uses 16-bit commands. Protocol:
 * Start -> Addr(W) -> CmdMSB -> CmdLSB -> Stop
 * 
 * Read Logic:
 * Start -> Addr(W) -> CmdMSB -> CmdLSB -> Stop
 * Delay (Measure time)
 * Start -> Addr(R) -> DataMSB -> DataLSB -> CRC -> DataMSB -> DataLSB -> CRC ...
 * 
 * NOTE: The generic write_reg/read_reg usually implies (RegisterAddr, Data).
 * But SHT31 commands are 16-bit. 
 * If our platform_write takes 8-bit reg address, we need to adapt.
 * A common trick for 16-bit reg devices on 8-bit reg abstraction:
 * Pass MSB as 'reg' and LSB as first byte of 'data' ? Or just treat standard I2C write.
 * 
 * Let's assume write_reg(handle, reg, buf, len):
 * If we treat the whole command as data payload with NO register address in the I2C frame?
 * Or is it Register Address?
 * 
 * Standard HAL I2C Mem Write uses MemAdd (Register Address). 
 * For 16-bit register address, we can set MemAddSize to 16BIT.
 * 
 * But to keep it generic, we often wrap things.
 * Let's assume the platform implementation can handle the 16-bit command if we pass it correctly.
 * However, specific standard here:
 * ctx->write_reg(handle, reg_addr_8bit, data_buf, len)
 * 
 * SHT31 commands are essentially "Write 2 bytes".
 * So we can call: ctx->write_reg(handle, CMD_MSB, &CMD_LSB, 1) ? 
 * If platform_write does: Start -> Addr -> Reg(CMD_MSB) -> Buf[0](CMD_LSB) -> Stop.
 * Yes, that works.
 */

static uint8_t crc8(const uint8_t *data, int len) {
    const uint8_t POLYNOMIAL = 0x31;
    uint8_t crc = 0xFF;
    for (int j = len; j; --j) {
        crc ^= *data++;
        for (int i = 8; i; --i) {
            crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
        }
    }
    return crc;
}

// State definitions
#define SHT_IDLE 0
#define SHT_WAIT 1

int32_t SHT31_Init(sht31_ctx_t *ctx) {
    if (!ctx->address) ctx->address = SHT31_I2C_ADDR_DEFAULT;
    
    // Reset state
    ctx->state = SHT_IDLE;
    ctx->tick_start = 0;
    
    return SHT31_Reset(ctx);
}

int32_t SHT31_Reset(sht31_ctx_t *ctx) {
    uint8_t cmd_lsb = SHT31_SOFTRESET & 0xFF;
    return ctx->write_reg(ctx->handle, SHT31_SOFTRESET >> 8, &cmd_lsb, 1);
}

int32_t SHT31_ReadTempHum(sht31_ctx_t *ctx, float *temp_c, float *rh) {
    uint32_t now = HAL_GetTick();

    switch (ctx->state) {
        case SHT_IDLE:
        {
            // 1. Send Measure Command
            uint8_t cmd_lsb = SHT31_MEAS_HIGHREP & 0xFF;
            if (ctx->write_reg(ctx->handle, SHT31_MEAS_HIGHREP >> 8, &cmd_lsb, 1) != 0) return SHT31_ERROR;
            
            ctx->tick_start = now;
            ctx->state = SHT_WAIT;
            return SHT31_BUSY;
        }

        case SHT_WAIT:
        {
            // 2. Wait 15ms (High Repeatability Measurement Time)
            // If clock stretching was enabled, read attempt would block.
            // Since we use non-blocking here, we must rely on timer.
            if ((now - ctx->tick_start) < 15) return SHT31_BUSY;

            // 3. Read 6 Bytes
            uint8_t buf[6];
            // Passing 0 as reg address (dummy) as discussed previously
            if (ctx->read_reg(ctx->handle, 0, buf, 6) != 0) {
                // Read failed (maybe NACK if not ready?), reset to retry
                ctx->state = SHT_IDLE;
                return SHT31_ERROR;
            }

            // Check CRC
            if (buf[2] != crc8(buf, 2) || buf[5] != crc8(buf + 3, 2)) {
                 // CRC Fail
                 ctx->state = SHT_IDLE;
                 return SHT31_ERROR;
            }

            uint16_t st = (buf[0] << 8) | buf[1];
            uint16_t sh = (buf[3] << 8) | buf[4];

            *temp_c = -45.0f + (175.0f * (float)st / 65535.0f);
            *rh = 100.0f * (float)sh / 65535.0f;
            
            // Success, go back to IDLE for next cycle
            ctx->state = SHT_IDLE;
            return SHT31_OK;
        }
        
        default:
            ctx->state = SHT_IDLE;
            return SHT31_ERROR;
    }
}

int32_t SHT31_SetHeater(sht31_ctx_t *ctx, bool enable) {
    uint8_t cmd_lsb;
    uint8_t cmd_msb;

    if (enable) {
        cmd_msb = (uint8_t)(SHT31_HEATEREN >> 8);
        cmd_lsb = (uint8_t)(SHT31_HEATEREN & 0xFF);
    } else {
        cmd_msb = (uint8_t)(SHT31_HEATERDIS >> 8);
        cmd_lsb = (uint8_t)(SHT31_HEATERDIS & 0xFF);
    }

    return ctx->write_reg(ctx->handle, cmd_msb, &cmd_lsb, 1);
}

