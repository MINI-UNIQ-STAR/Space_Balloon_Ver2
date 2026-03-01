#include "mcp9600_driver.h"

// Note on MCP9600 I2C Read:
// Standard Register Read: Write Reg Addr -> Restart -> Read 2 bytes (Big Endian usually? Adafruit says MSBFIRST)

int32_t MCP9600_Init(mcp9600_ctx_t *ctx) {
    if (!ctx->address) ctx->address = MCP9600_I2C_ADDR_DEFAULT;

    // Config: Type K, Filter Mid
    uint8_t config = (MCP9600_TYPE_K << 4) | MCP9600_FILTER_MID; 
    if (ctx->write_reg(ctx->handle, MCP9600_REG_SENSOR_CONFIG, &config, 1) != 0) return -1;
    
    // Device Config: Normal Mode (0x00)
    // Adafruit writes 0x80 (1000 0000) ?? bit 7 is ignored? 
    // Datasheet: Bit 1-0: 00 = Normal, 01 = Shutdown, 10 = Burst, 11 = Reserved.
    // Adafruit uses 0x00 for Active in `enable(true)`. 
    uint8_t dev_conf = 0x00; 
    return ctx->write_reg(ctx->handle, MCP9600_REG_DEVICE_CONFIG, &dev_conf, 1);
}

int32_t MCP9600_ReadThermocouple(mcp9600_ctx_t *ctx, float *temp_c) {
    uint8_t buf[2];
    if (ctx->read_reg(ctx->handle, MCP9600_REG_HOT_JUNCTION, buf, 2) != 0) return -1;
    
    int16_t val = ((int16_t)buf[0] << 8) | buf[1];
    *temp_c = val * 0.0625f;
    return 0;
}

int32_t MCP9600_ReadAmbient(mcp9600_ctx_t *ctx, float *temp_c) {
    uint8_t buf[2];
    if (ctx->read_reg(ctx->handle, MCP9600_REG_COLD_JUNCTION, buf, 2) != 0) return -1;
    
    int16_t val = ((int16_t)buf[0] << 8) | buf[1];
    *temp_c = val * 0.0625f;
    return 0;
}
