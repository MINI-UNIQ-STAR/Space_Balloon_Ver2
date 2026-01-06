#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool valid;
	uint16_t pm1_ugm3;
	uint16_t pm25_ugm3;
	uint16_t pm10_ugm3;
} pms_reading_t;

typedef struct {
	uint8_t buf[64];
	size_t idx;
	uint16_t frame_len; // length field value
} pms_parser_t;

void pms_parser_init(pms_parser_t *p);

// Feed raw bytes from UART stream.
// Returns true if at least one complete frame was parsed (and state updated).
bool pms_parser_feed(pms_parser_t *p, const uint8_t *data, size_t len, pms_reading_t *out_latest);

#ifdef __cplusplus
}
#endif
