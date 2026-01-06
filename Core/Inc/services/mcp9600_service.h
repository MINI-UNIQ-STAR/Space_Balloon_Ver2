#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void mcp9600_service_init(void);
void mcp9600_service_tick(uint32_t now_ms);

void mcp9600_service_reset(void);

bool mcp9600_service_get_last_update_ms(uint32_t *out_ms);

// Returns MCP9600 cold junction (internal/ambient) temperature in °C*100.
// Returns false if no valid reading is available.
bool mcp9600_service_get_cold_junction_c_x100(int32_t *out_c_x100);

// Returns MCP9600 hot junction (thermocouple/external) temperature in °C*100.
// Returns false if no valid reading is available.
bool mcp9600_service_get_hot_junction_c_x100(int32_t *out_c_x100);

#ifdef __cplusplus
}
#endif
