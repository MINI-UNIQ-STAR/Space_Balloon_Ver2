#include "services/telemetry_frame.h"

static void write_u16_le(uint8_t *dst, uint16_t v)
{
	dst[0] = (uint8_t)(v & 0xFFu);
	dst[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void write_u32_le(uint8_t *dst, uint32_t v)
{
	dst[0] = (uint8_t)(v & 0xFFu);
	dst[1] = (uint8_t)((v >> 8) & 0xFFu);
	dst[2] = (uint8_t)((v >> 16) & 0xFFu);
	dst[3] = (uint8_t)((v >> 24) & 0xFFu);
}

uint16_t telemetry_crc16_ccitt_false(const uint8_t *data, size_t len)
{
	// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, xorout 0x0000, refin/out=false
	uint16_t crc = 0xFFFFu;
	for (size_t i = 0; i < len; i++) {
		crc ^= (uint16_t)data[i] << 8;
		for (uint8_t b = 0; b < 8; b++) {
			if ((crc & 0x8000u) != 0u) {
				crc = (uint16_t)((crc << 1) ^ 0x1021u);
			} else {
				crc = (uint16_t)(crc << 1);
			}
		}
	}
	return crc;
}

size_t telemetry_build_frame(uint8_t msg_type,
						 const uint8_t *payload,
						 size_t payload_len,
						 uint16_t seq,
						 uint32_t timestamp_ms,
						 uint8_t *out,
						 size_t out_cap)
{
	const size_t header_len = 12;
	const size_t crc_len = 2;
	if (out == NULL) {
		return 0;
	}
	if ((payload_len > 0u) && (payload == NULL)) {
		return 0;
	}
	if (payload_len > 0xFFFFu) {
		return 0;
	}

	const size_t total = header_len + payload_len + crc_len;
	if (out_cap < total) {
		return 0;
	}

	out[0] = TELEMETRY_MAGIC0;
	out[1] = TELEMETRY_MAGIC1;
	out[2] = TELEMETRY_VERSION;
	out[3] = msg_type;
	write_u16_le(&out[4], (uint16_t)payload_len);
	write_u16_le(&out[6], seq);
	write_u32_le(&out[8], timestamp_ms);

	for (size_t i = 0; i < payload_len; i++) {
		out[header_len + i] = payload[i];
	}

	const uint16_t crc = telemetry_crc16_ccitt_false(out, header_len + payload_len);
	write_u16_le(&out[header_len + payload_len], crc);

	return total;
}
