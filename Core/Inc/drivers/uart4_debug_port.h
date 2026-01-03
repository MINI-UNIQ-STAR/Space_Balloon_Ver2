#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Low-level UART4 debug output port.
// NOTE: This project reserves UART3 for ESP32 (LoRa32) framing, so debug must not use UART3.
// UART4 is used here only when explicitly enabled.
#ifndef UART4_DEBUG_PORT_ENABLE
#define UART4_DEBUG_PORT_ENABLE 0
#endif

bool uart4_debug_port_init(void);
bool uart4_debug_port_write(const void *data, size_t len);

#ifdef __cplusplus
}
#endif
