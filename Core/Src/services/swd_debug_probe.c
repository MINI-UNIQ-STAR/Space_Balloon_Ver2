#include "services/swd_debug_probe.h"

#include <stdbool.h>

#include "drivers/gps_int_capture.h"
#include "services/health_monitor_service.h"

#if SWD_DEBUG_PROBE_ENABLE

// These symbols are intended to be viewed over SWD using a debugger.
// Keep them volatile to prevent optimization removing or caching values.
__attribute__((used)) volatile uint32_t g_swd_dbg_now_ms = 0;
__attribute__((used)) volatile uint16_t g_swd_dbg_health_flags = 0;
__attribute__((used)) volatile uint16_t g_swd_dbg_health_ext_flags = 0;

__attribute__((used)) volatile uint32_t g_swd_dbg_gps_int_count = 0;
__attribute__((used)) volatile uint32_t g_swd_dbg_gps_int_last_ms = 0;
__attribute__((used)) volatile uint32_t g_swd_dbg_gps_int_age_ms = 0xFFFFFFFFu;

static uint32_t s_next_update_ms = 0;

void swd_debug_probe_init(void)
{
	s_next_update_ms = 0;
	g_swd_dbg_now_ms = 0;
	g_swd_dbg_health_flags = 0;
	g_swd_dbg_health_ext_flags = 0;
	g_swd_dbg_gps_int_count = 0;
	g_swd_dbg_gps_int_last_ms = 0;
	g_swd_dbg_gps_int_age_ms = 0xFFFFFFFFu;
}

void swd_debug_probe_tick(uint32_t now_ms)
{
	g_swd_dbg_now_ms = now_ms;

	if ((int32_t)(now_ms - s_next_update_ms) < 0) {
		return;
	}
	s_next_update_ms = now_ms + 1000u;

	g_swd_dbg_health_flags = health_monitor_get_flags();
	g_swd_dbg_health_ext_flags = health_monitor_get_ext_flags();

	uint32_t last_ms = 0;
	uint32_t count = 0;
	bool has_last = gps_int_capture_get_last(&last_ms, &count);
	g_swd_dbg_gps_int_count = count;
	if (has_last) {
		g_swd_dbg_gps_int_last_ms = last_ms;
		g_swd_dbg_gps_int_age_ms = now_ms - last_ms;
	} else {
		g_swd_dbg_gps_int_last_ms = 0;
		g_swd_dbg_gps_int_age_ms = 0xFFFFFFFFu;
	}
}

#else

void swd_debug_probe_init(void)
{
}

void swd_debug_probe_tick(uint32_t now_ms)
{
	(void)now_ms;
}

#endif
