#include "services/pms3003_service.h"

#include "drivers/uart_rx_poll.h"

#include <string.h>

static pms_parser_t s_parser;
static bool s_valid = false;
static pms_reading_t s_last = {0};
static uint32_t s_last_update_ms = 0;

void pms3003_service_init(void)
{
	pms_parser_init(&s_parser);
	s_valid = false;
	memset(&s_last, 0, sizeof(s_last));
	s_last_update_ms = 0;
}

void pms3003_service_reset(void)
{
	pms3003_service_init();
}

void pms3003_service_tick(uint32_t now_ms)
{
	uint8_t buf[64];
	size_t n = 0;
	if (!uart2_rx_poll_read(buf, sizeof(buf), &n)) {
		return;
	}
	if (n == 0u) {
		return;
	}

	pms_reading_t r;
	if (pms_parser_feed(&s_parser, buf, n, &r) && r.valid) {
		s_last = r;
		s_valid = true;
		s_last_update_ms = now_ms;
	}
}

bool pms3003_get_reading(pms_reading_t *out)
{
	if ((out == NULL) || !s_valid) {
		return false;
	}
	*out = s_last;
	return true;
}

bool pms3003_service_get_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_valid) {
		return false;
	}
	*out_ms = s_last_update_ms;
	return true;
}
