#include "services/gps_service.h"

#include "drivers/uart_rx_poll.h"
#include "drivers/uart_tx.h"
#include "drivers/gps_ctrl.h"
#include "drivers/gps_int_capture.h"

#include "stm32g4xx_hal.h"

#include <string.h>

static nmea_parser_t s_parser;
static bool s_has_any = false;
static uint32_t s_last_rx_ms = 0;

static bool s_cfg_pending = false;
static uint32_t s_cfg_send_at_ms = 0;

static uint8_t hex_nibble(uint8_t v)
{
	v &= 0x0Fu;
	return (v < 10u) ? (uint8_t)('0' + v) : (uint8_t)('A' + (v - 10u));
}

static bool gps_send_pmtk_payload(const char *payload)
{
	if (payload == NULL) {
		return false;
	}

	// Build: $<payload>*HH\r\n
	// Checksum is XOR of bytes between '$' and '*'.
	char buf[64];
	size_t payload_len = strlen(payload);
	if (payload_len == 0u) {
		return false;
	}
	if (payload_len > 50u) {
		return false;
	}

	uint8_t cksum = 0;
	for (size_t i = 0; i < payload_len; i++) {
		cksum ^= (uint8_t)payload[i];
	}

	size_t idx = 0;
	buf[idx++] = '$';
	memcpy(&buf[idx], payload, payload_len);
	idx += payload_len;
	buf[idx++] = '*';
	buf[idx++] = (char)hex_nibble((uint8_t)(cksum >> 4));
	buf[idx++] = (char)hex_nibble((uint8_t)(cksum & 0x0Fu));
	buf[idx++] = '\r';
	buf[idx++] = '\n';

	return uart1_tx_write((const uint8_t *)buf, idx, 100);
}

void gps_service_init(void)
{
	gps_ctrl_init();
	gps_int_capture_init();

	nmea_parser_init(&s_parser);
	s_has_any = false;
	s_last_rx_ms = 0;

	// Titan X1 (MT3333) supports MTK PMTK command protocol.
	// Schedule a one-shot command to set update rate to 10Hz (100ms).
	// Datasheet: Update Rate up to 10Hz.
	s_cfg_pending = true;
	s_cfg_send_at_ms = HAL_GetTick() + 500u;
}

void gps_service_reset(void)
{
	gps_service_init();
}

void gps_service_tick(uint32_t now_ms)
{
	if (s_cfg_pending && ((int32_t)(now_ms - s_cfg_send_at_ms) >= 0)) {
		// Set NMEA update rate to 100ms (10Hz)
		(void)gps_send_pmtk_payload("PMTK220,100");
		s_cfg_pending = false;
	}

	// Read a small chunk per tick to keep CPU predictable.
	uint8_t buf[64];
	size_t n = 0;
	if (!uart1_rx_poll_read(buf, sizeof(buf), &n)) {
		return;
	}
	if (n == 0u) {
		return;
	}

	nmea_parser_feed(&s_parser, buf, n);
	s_has_any = true;
	s_last_rx_ms = now_ms;
}

bool gps_service_get_state(nmea_gps_state_t *out)
{
	if (out == NULL) {
		return false;
	}
	memcpy(out, &s_parser.state, sizeof(*out));
	return s_has_any;
}

bool gps_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_has_any) {
		return false;
	}
	*out_ms = s_last_rx_ms;
	return true;
}
