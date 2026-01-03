#include "drivers/pms_parser.h"

#include <string.h>

static uint16_t be16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static bool parse_frame(const uint8_t *frame, size_t frame_len, pms_reading_t *out)
{
	// Frame format (typical PMSx003):
	// [0..1]  0x42 0x4D
	// [2..3]  length (big-endian), number of bytes after this field
	// [4..]   payload (length bytes) including checksum at the end
	// checksum: sum of bytes [0..(frame_len-3)] (big-endian at last 2 bytes)
	if ((frame == NULL) || (out == NULL) || (frame_len < 8u)) {
		return false;
	}
	if (frame[0] != 0x42u || frame[1] != 0x4Du) {
		return false;
	}
	const uint16_t len = be16(&frame[2]);
	if ((size_t)(4u + len) != frame_len) {
		return false;
	}
	if (len < 2u) {
		return false;
	}

	uint32_t sum = 0;
	for (size_t i = 0; i + 2u < frame_len; i++) {
		sum += frame[i];
	}
	const uint16_t chk = be16(&frame[frame_len - 2u]);
	if (((uint16_t)sum) != chk) {
		return false;
	}

	// Try to decode atmospheric PM values at standard offsets.
	// Payload starts at [4]. For PMSx003, atmospheric:
	// PM1.0 at [4+6..7], PM2.5 at [4+8..9], PM10 at [4+10..11]
	if (frame_len < (4u + 12u + 2u)) {
		return false;
	}
	out->pm1_ugm3 = be16(&frame[4u + 6u]);
	out->pm25_ugm3 = be16(&frame[4u + 8u]);
	out->pm10_ugm3 = be16(&frame[4u + 10u]);
	out->valid = true;
	return true;
}

void pms_parser_init(pms_parser_t *p)
{
	if (p == NULL) {
		return;
	}
	memset(p, 0, sizeof(*p));
}

static void parser_reset(pms_parser_t *p)
{
	p->idx = 0;
	p->frame_len = 0;
}

bool pms_parser_feed(pms_parser_t *p, const uint8_t *data, size_t len, pms_reading_t *out_latest)
{
	if ((p == NULL) || (data == NULL) || (len == 0u)) {
		return false;
	}

	bool any = false;
	for (size_t i = 0; i < len; i++) {
		const uint8_t b = data[i];

		// Sync to header 0x42 0x4D
		if (p->idx == 0u) {
			if (b != 0x42u) {
				continue;
			}
			p->buf[p->idx++] = b;
			continue;
		}
		if (p->idx == 1u) {
			if (b != 0x4Du) {
				parser_reset(p);
				continue;
			}
			p->buf[p->idx++] = b;
			continue;
		}

		// Collect length field
		if (p->idx < 4u) {
			p->buf[p->idx++] = b;
			if (p->idx == 4u) {
				p->frame_len = be16(&p->buf[2]);
				// total bytes = 4 + frame_len
				if (p->frame_len == 0u || (4u + p->frame_len) > sizeof(p->buf)) {
					parser_reset(p);
				}
			}
			continue;
		}

		p->buf[p->idx++] = b;
		if ((p->frame_len > 0u) && (p->idx == (size_t)(4u + p->frame_len))) {
			pms_reading_t r = {0};
			if (parse_frame(p->buf, p->idx, &r)) {
				any = true;
				if (out_latest != NULL) {
					*out_latest = r;
				}
			}
			parser_reset(p);
		}

		if (p->idx >= sizeof(p->buf)) {
			parser_reset(p);
		}
	}

	return any;
}
