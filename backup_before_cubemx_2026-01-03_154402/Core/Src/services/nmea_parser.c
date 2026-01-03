#include "services/nmea_parser.h"

#include <string.h>

static bool is_digit(char c)
{
	return (c >= '0') && (c <= '9');
}

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9') {
		return (int)(c - '0');
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + (int)(c - 'A');
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + (int)(c - 'a');
	}
	return -1;
}

static bool nmea_checksum_ok(const char *line)
{
	// If a checksum is present (*HH), validate it.
	// If no checksum delimiter is present, accept the sentence.
	if (line == NULL) {
		return false;
	}
	const char *star = strchr(line, '*');
	if (star == NULL) {
		return true;
	}
	if ((star[1] == '\0') || (star[2] == '\0')) {
		return false;
	}
	const int hi = hex_nibble(star[1]);
	const int lo = hex_nibble(star[2]);
	if ((hi < 0) || (lo < 0)) {
		return false;
	}
	const uint8_t expected = (uint8_t)((hi << 4) | lo);

	uint8_t cs = 0;
	// NMEA checksum is XOR of bytes between '$' and '*'
	for (const char *p = line; *p != '\0' && *p != '*'; p++) {
		if (*p == '$') {
			continue;
		}
		cs ^= (uint8_t)(*p);
	}

	return cs == expected;
}

static int parse_u8(const char *s)
{
	int v = 0;
	if ((s == NULL) || (s[0] == '\0')) {
		return -1;
	}
	for (size_t i = 0; s[i] != '\0'; i++) {
		if (!is_digit(s[i])) {
			return -1;
		}
		v = (v * 10) + (s[i] - '0');
		if (v > 255) {
			return -1;
		}
	}
	return v;
}

static int64_t parse_minutes_to_deg_e7(uint32_t deg, const char *min_str)
{
	// min_str is like "mm.mmmm" or "m.mmmm" etc (for GGA it's typically 2 digits + '.' + 4 digits)
	// We convert minutes to degrees: deg + minutes/60.
	// We compute degrees_e7 = deg*1e7 + round(min_x1e4 * 1e7 / (60*1e4)).
	if ((min_str == NULL) || (min_str[0] == '\0')) {
		return (int64_t)deg * 10000000LL;
	}

	uint32_t int_part = 0;
	uint32_t frac_part = 0;
	uint32_t frac_scale = 1;
	bool seen_dot = false;

	for (size_t i = 0; min_str[i] != '\0'; i++) {
		char c = min_str[i];
		if (c == '.') {
			seen_dot = true;
			continue;
		}
		if (!is_digit(c)) {
			break;
		}
		if (!seen_dot) {
			int_part = (int_part * 10u) + (uint32_t)(c - '0');
		} else {
			if (frac_scale < 10000u) {
				frac_part = (frac_part * 10u) + (uint32_t)(c - '0');
				frac_scale *= 10u;
			}
		}
	}

	// Scale fractional minutes to 1e4.
	while (frac_scale < 10000u) {
		frac_part *= 10u;
		frac_scale *= 10u;
	}

	const uint32_t min_x1e4 = (int_part * 10000u) + frac_part;
	const int64_t base = (int64_t)deg * 10000000LL;
	// Rounded division
	const int64_t num = (int64_t)min_x1e4 * 10000000LL + 300000LL; // +den/2 where den=600000
	const int64_t add = num / 600000LL;
	return base + add;
}

static bool parse_lat_lon_deg_e7(const char *coord, const char *hemi, bool is_lat, int32_t *out_deg_e7)
{
	if ((coord == NULL) || (hemi == NULL) || (out_deg_e7 == NULL) || (coord[0] == '\0') || (hemi[0] == '\0')) {
		return false;
	}

	// lat: ddmm.mmmm, lon: dddmm.mmmm
	const size_t deg_digits = is_lat ? 2u : 3u;
	for (size_t i = 0; i < deg_digits; i++) {
		if (!is_digit(coord[i])) {
			return false;
		}
	}

	uint32_t deg = 0;
	for (size_t i = 0; i < deg_digits; i++) {
		deg = (deg * 10u) + (uint32_t)(coord[i] - '0');
	}

	const char *min_str = coord + deg_digits;
	int64_t deg_e7 = parse_minutes_to_deg_e7(deg, min_str);

	char h = hemi[0];
	if (is_lat) {
		if (h == 'S') {
			deg_e7 = -deg_e7;
		} else if (h != 'N') {
			return false;
		}
	} else {
		if (h == 'W') {
			deg_e7 = -deg_e7;
		} else if (h != 'E') {
			return false;
		}
	}

	if (deg_e7 < (int64_t)INT32_MIN || deg_e7 > (int64_t)INT32_MAX) {
		return false;
	}

	*out_deg_e7 = (int32_t)deg_e7;
	return true;
}

static bool parse_decimal_scaled_i32(const char *s, int32_t scale, int32_t *out)
{
	// Parses a decimal number into fixed-point integer with given scale.
	// Example: "12.34" with scale=1000 -> 12340.
	if ((s == NULL) || (out == NULL) || (s[0] == '\0')) {
		return false;
	}

	bool neg = false;
	size_t i = 0;
	if (s[i] == '-') {
		neg = true;
		i++;
	}

	int64_t int_part = 0;
	while (s[i] != '\0' && s[i] != '.') {
		if (!is_digit(s[i])) {
			return false;
		}
		int_part = (int_part * 10LL) + (int64_t)(s[i] - '0');
		i++;
	}

	int64_t frac_part = 0;
	int64_t frac_scale = 1;
	if (s[i] == '.') {
		i++;
		while (s[i] != '\0' && frac_scale < scale) {
			if (!is_digit(s[i])) {
				break;
			}
			frac_part = (frac_part * 10LL) + (int64_t)(s[i] - '0');
			frac_scale *= 10;
			i++;
		}
	}

	while (frac_scale < scale) {
		frac_part *= 10;
		frac_scale *= 10;
	}

	int64_t v = int_part * (int64_t)scale + frac_part;
	if (neg) {
		v = -v;
	}
	if (v < (int64_t)INT32_MIN || v > (int64_t)INT32_MAX) {
		return false;
	}
	*out = (int32_t)v;
	return true;
}

static void update_gsv_counts(nmea_gps_state_t *st, const char *talker, int total_in_view)
{
	if ((st == NULL) || (talker == NULL) || (total_in_view < 0)) {
		return;
	}
	if (total_in_view > 255) {
		total_in_view = 255;
	}

	// talker is 2 chars after '$': GP/GL/GA/GB/BD/GN...
	if ((talker[0] == 'G') && (talker[1] == 'P')) {
		st->sats_in_view_gps = (uint8_t)total_in_view;
	} else if ((talker[0] == 'G') && (talker[1] == 'L')) {
		st->sats_in_view_glonass = (uint8_t)total_in_view;
	} else if ((talker[0] == 'G') && (talker[1] == 'A')) {
		st->sats_in_view_galileo = (uint8_t)total_in_view;
	} else if (((talker[0] == 'G') && (talker[1] == 'B')) || ((talker[0] == 'B') && (talker[1] == 'D'))) {
		st->sats_in_view_beidou = (uint8_t)total_in_view;
	} else if ((talker[0] == 'G') && (talker[1] == 'N')) {
		// Combined GNSS
		st->sats_in_view_total = (uint8_t)total_in_view;
		return;
	}

	// Derive a total if we have per-constellation numbers.
	const uint16_t sum = (uint16_t)st->sats_in_view_gps + (uint16_t)st->sats_in_view_glonass +
					 (uint16_t)st->sats_in_view_galileo + (uint16_t)st->sats_in_view_beidou;
	if (sum > 0u) {
		st->sats_in_view_total = (uint8_t)((sum > 255u) ? 255u : sum);
	}
}

static void parse_sentence(nmea_parser_t *p, char *line)
{
	// line is mutable, no CRLF, no leading/trailing spaces.
	if ((p == NULL) || (line == NULL)) {
		return;
	}

	if (line[0] != '$') {
		return;
	}

	if (!nmea_checksum_ok(line)) {
		return;
	}

	// Strip checksum part: *XX
	char *star = strchr(line, '*');
	if (star != NULL) {
		*star = '\0';
	}

	// Header is like $GPGGA or $GNGSV
	if (strlen(line) < 6u) {
		return;
	}

	const char talker0 = line[1];
	const char talker1 = line[2];
	const char type0 = line[3];
	const char type1 = line[4];
	const char type2 = line[5];
	const char talker[3] = {talker0, talker1, '\0'};

	// Split CSV fields in-place
	char *fields[20] = {0};
	int nf = 0;
	fields[nf++] = line; // includes header
	for (char *c = line; *c != '\0' && nf < (int)(sizeof(fields) / sizeof(fields[0])); c++) {
		if (*c == ',') {
			*c = '\0';
			fields[nf++] = c + 1;
		}
	}

	// $--GGA: fields we need:
	// [2]=lat, [3]=N/S, [4]=lon, [5]=E/W, [6]=fix quality, [7]=sats used, [9]=alt (m)
	if ((type0 == 'G') && (type1 == 'G') && (type2 == 'A')) {
		if (nf < 10) {
			return;
		}
		const int fixq = parse_u8(fields[6]);
		const int sats = parse_u8(fields[7]);
		int32_t lat_e7 = 0;
		int32_t lon_e7 = 0;
		int32_t alt_mm = 0;

		bool ok = true;
		ok = ok && parse_lat_lon_deg_e7(fields[2], fields[3], true, &lat_e7);
		ok = ok && parse_lat_lon_deg_e7(fields[4], fields[5], false, &lon_e7);
		ok = ok && parse_decimal_scaled_i32(fields[9], 1000, &alt_mm);
		if (!ok) {
			return;
		}

		p->state.has_fix = (fixq > 0);
		p->state.lat_deg_e7 = lat_e7;
		p->state.lon_deg_e7 = lon_e7;
		p->state.alt_mm = alt_mm;
		p->state.sats_used = (uint8_t)((sats < 0) ? 0 : (sats > 255 ? 255 : sats));
		return;
	}

	// $--RMC: fields we need:
	// [2]=status (A=active, V=void)
	// [3]=lat, [4]=N/S, [5]=lon, [6]=E/W
	if ((type0 == 'R') && (type1 == 'M') && (type2 == 'C')) {
		if (nf < 7) {
			return;
		}
		const char *status = fields[2];
		int32_t lat_e7 = 0;
		int32_t lon_e7 = 0;

		bool ok = true;
		ok = ok && (status != NULL) && (status[0] == 'A' || status[0] == 'V');
		ok = ok && parse_lat_lon_deg_e7(fields[3], fields[4], true, &lat_e7);
		ok = ok && parse_lat_lon_deg_e7(fields[5], fields[6], false, &lon_e7);
		if (!ok) {
			return;
		}

		p->state.has_fix = (status[0] == 'A');
		p->state.lat_deg_e7 = lat_e7;
		p->state.lon_deg_e7 = lon_e7;
		return;
	}

	// $--GSV: fields we need:
	// [3]=total satellites in view
	if ((type0 == 'G') && (type1 == 'S') && (type2 == 'V')) {
		if (nf < 4) {
			return;
		}
		const int total = parse_u8(fields[3]);
		update_gsv_counts(&p->state, talker, total);
		return;
	}
}

void nmea_parser_init(nmea_parser_t *p)
{
	if (p == NULL) {
		return;
	}
	memset(p, 0, sizeof(*p));
}

void nmea_parser_feed(nmea_parser_t *p, const uint8_t *data, size_t len)
{
	if ((p == NULL) || (data == NULL) || (len == 0u)) {
		return;
	}

	for (size_t i = 0; i < len; i++) {
		char c = (char)data[i];
		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			p->line[p->line_len] = '\0';
			if (p->line_len > 0u) {
				parse_sentence(p, p->line);
			}
			p->line_len = 0;
			continue;
		}

		if (p->line_len + 1u < sizeof(p->line)) {
			p->line[p->line_len++] = c;
		} else {
			// overflow: drop line
			p->line_len = 0;
		}
	}
}
