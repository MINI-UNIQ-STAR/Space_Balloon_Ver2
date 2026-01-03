#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Polls USART1 and reads up to max_len bytes.
// Returns true if the call succeeded (including "no data" case).
// out_len is set to the number of bytes read (0..max_len).
bool uart1_rx_poll_read(uint8_t *buf, size_t max_len, size_t *out_len);

// Polls USART2 and reads up to max_len bytes.
// Returns true if the call succeeded (including "no data" case).
// out_len is set to the number of bytes read (0..max_len).
bool uart2_rx_poll_read(uint8_t *buf, size_t max_len, size_t *out_len);

#ifdef __cplusplus
}
#endif
