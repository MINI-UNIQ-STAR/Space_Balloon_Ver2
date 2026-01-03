#ifndef MCP9600_DRIVER_H
#define MCP9600_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reads MCP9600 cold junction (internal/ambient) temperature.
//
// Output unit: centi-degrees Celsius (°C * 100).
// Returns false on I2C error.
bool mcp9600_read_cold_junction_c_x100(int32_t *out_c_x100);

#ifdef __cplusplus
}
#endif

#endif /* MCP9600_DRIVER_H */
