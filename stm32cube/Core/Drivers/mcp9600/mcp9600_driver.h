#ifndef MCP9600_DRIVER_H
#define MCP9600_DRIVER_H

#include <stdint.h>

#define MCP9600_I2C_ADDR_DEFAULT    0x67 // or 0x60 depending on pins

#define MCP9600_REG_HOT_JUNCTION    0x00
#define MCP9600_REG_JUNCTION_DELTA  0x01
#define MCP9600_REG_COLD_JUNCTION   0x02
#define MCP9600_REG_RAW_ADC         0x03
#define MCP9600_REG_STATUS          0x04
#define MCP9600_REG_SENSOR_CONFIG   0x05
#define MCP9600_REG_DEVICE_CONFIG   0x06

// Thermocouple Types
#define MCP9600_TYPE_K              0x00
#define MCP9600_TYPE_J              0x01
#define MCP9600_TYPE_T              0x02
#define MCP9600_TYPE_N              0x03
#define MCP9600_TYPE_S              0x04
#define MCP9600_TYPE_E              0x05
#define MCP9600_TYPE_B              0x06
#define MCP9600_TYPE_R              0x07

// Filter Coefficients
#define MCP9600_FILTER_OFF          0x00
#define MCP9600_FILTER_MIN          0x01
#define MCP9600_FILTER_MID          0x04
#define MCP9600_FILTER_MAX          0x07

typedef int32_t (*mcp9600_write_ptr)(void *, uint8_t, const uint8_t *, uint16_t);
typedef int32_t (*mcp9600_read_ptr)(void *, uint8_t, uint8_t *, uint16_t);

typedef struct {
    mcp9600_write_ptr write_reg;
    mcp9600_read_ptr read_reg;
    void *handle;
    uint8_t address;
} mcp9600_ctx_t;

int32_t MCP9600_Init(mcp9600_ctx_t *ctx);
int32_t MCP9600_ReadThermocouple(mcp9600_ctx_t *ctx, float *temp_c);
int32_t MCP9600_ReadAmbient(mcp9600_ctx_t *ctx, float *temp_c);

#endif
