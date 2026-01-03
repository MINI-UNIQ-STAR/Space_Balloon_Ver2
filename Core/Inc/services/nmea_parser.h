#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool has_fix;
	int32_t lat_deg_e7; // degrees * 1e7
	int32_t lon_deg_e7; // degrees * 1e7
	int32_t alt_mm;     // meters * 1000
	uint8_t sats_used;
	uint16_t hdop_x100; // HDOP * 100 (e.g. 1.23 -> 123)

	// Satellites in view (from GSV). Not all constellations may be provided.
	uint8_t sats_in_view_total;
	uint8_t sats_in_view_gps;
	uint8_t sats_in_view_glonass;
	uint8_t sats_in_view_galileo;
	uint8_t sats_in_view_beidou;
} nmea_gps_state_t;

typedef struct {
	nmea_gps_state_t state;

	char line[128];
	size_t line_len;
} nmea_parser_t;

void nmea_parser_init(nmea_parser_t *p);

// Feed raw bytes (UART stream). Parses complete lines ending in \n.
void nmea_parser_feed(nmea_parser_t *p, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
