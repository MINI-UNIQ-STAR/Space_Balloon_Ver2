#include "services/uart4_debug_log.h"

#include <stdio.h>

#include "drivers/gps_int_capture.h"
#include "drivers/uart4_debug_port.h"

#if UART4_DEBUG_LOG_ENABLE

static uint32_t s_next_log_ms = 0;

void uart4_debug_log_init(void)
{
	(void)uart4_debug_port_init();
	s_next_log_ms = HAL_GetTick();
}

void uart4_debug_log_tick(uint32_t now_ms)
{
	if ((int32_t)(now_ms - s_next_log_ms) < 0) {
		return;
	}

	s_next_log_ms = now_ms + 1000u;

	uint32_t last_ms = 0;
	uint32_t count = 0;
	bool has_last = gps_int_capture_get_last(&last_ms, &count);

	uint32_t age_ms = 0;
	if (has_last) {
		age_ms = now_ms - last_ms;
	} else {
		age_ms = 0xFFFFFFFFu;
	}

	char buf[64];
	int n = 0;
	if (has_last) {
		n = snprintf(buf, sizeof(buf), "GPS_INT cnt=%lu age=%lums\r\n", (unsigned long)count, (unsigned long)age_ms);
	} else {
		n = snprintf(buf, sizeof(buf), "GPS_INT cnt=%lu age=NA\r\n", (unsigned long)count);
	}

	if (n <= 0) {
		return;
	}
	if ((size_t)n > sizeof(buf)) {
		n = (int)sizeof(buf);
	}

	(void)uart4_debug_port_write(buf, (size_t)n);
}

#else

void uart4_debug_log_init(void)
{
	// disabled
}

void uart4_debug_log_tick(uint32_t now_ms)
{
	(void)now_ms;
	// disabled
}

#endif
