#ifndef MCP9600_PROBE_H
#define MCP9600_PROBE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if the MCP9600 responds on the I2C bus.
bool mcp9600_probe_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* MCP9600_PROBE_H */
