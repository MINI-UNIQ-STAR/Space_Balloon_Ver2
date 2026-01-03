#include "app/app.h"

#include "services/telemetry_service.h"
#include "services/health_monitor_service.h"
#include "services/gps_service.h"
#include "services/aux_sensors_service.h"
#include "services/air_quality_service.h"
#include "services/co2_service.h"
#include "services/ozone_service.h"
#include "services/heater_service.h"
#include "services/sht31_service.h"
#include "services/ms5611_service.h"
#include "services/gdk101_service.h"
#include "services/imu_service.h"
#include "services/alt_kf_service.h"
#include "services/mcp9600_service.h"
#include "services/mag_service.h"
#include "services/uart4_debug_log.h"
#include "services/swd_debug_probe.h"

void app_init(void)
{
	gps_service_init();
	aux_sensors_service_init();
	air_quality_service_init();
	co2_service_init();
	ozone_service_init();
	heater_service_init();
	gdk101_service_init();
	sht31_service_init();
	ms5611_service_init();
	imu_service_init();
	mag_service_init();
	mcp9600_service_init();
	alt_kf_service_init();
	uart4_debug_log_init();
	swd_debug_probe_init();

	health_monitor_service_init();
	telemetry_service_init();
}

void app_tick(uint32_t now_ms)
{
	// Prioritize latency-sensitive paths first (UART RX/poll, fast attitude, telemetry framing),
	// then run slower/blocking sensors. This reduces jitter when an I2C transaction stalls.
	gps_service_tick(now_ms);
	imu_service_tick(now_ms);
	uart4_debug_log_tick(now_ms);
	swd_debug_probe_tick(now_ms);
	telemetry_service_tick(now_ms);

	// Sensors / slower services (some may block on I2C/UART timeouts)
	aux_sensors_service_tick(now_ms);
	ms5611_service_tick(now_ms);
	sht31_service_tick(now_ms);
	mcp9600_service_tick(now_ms);
	mag_service_tick(now_ms);
	gdk101_service_tick(now_ms);
	co2_service_tick(now_ms);
	ozone_service_tick(now_ms);
	air_quality_service_tick(now_ms);
	heater_service_tick(now_ms);
	alt_kf_service_tick(now_ms);

	// Health monitor last so it observes the latest update timestamps.
	health_monitor_service_tick(now_ms);
}
