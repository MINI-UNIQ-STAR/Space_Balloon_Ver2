#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Health check:
// - checks every 1000ms
// - if no update for >=2000ms -> mark ERROR and attempt recovery
// - if recovery fails 5 times -> permanent fail until reboot

typedef enum {
	HEALTH_FLAG_TEMP_INT_STALE = (1u << 0),
	HEALTH_FLAG_GPS_STALE      = (1u << 1),
	HEALTH_FLAG_PMS_STALE      = (1u << 2),
	HEALTH_FLAG_SHT31_STALE    = (1u << 3),
	HEALTH_FLAG_MS5611_STALE   = (1u << 4),
	HEALTH_FLAG_GDK101_STALE   = (1u << 5),
	HEALTH_FLAG_IMU_STALE      = (1u << 6),
	HEALTH_FLAG_MS5611_ANOMALY = (1u << 7),

	HEALTH_FLAG_PERMFAIL_TEMP_INT = (1u << 8),
	HEALTH_FLAG_PERMFAIL_GPS      = (1u << 9),
	HEALTH_FLAG_PERMFAIL_PMS      = (1u << 10),
	HEALTH_FLAG_PERMFAIL_SHT31    = (1u << 11),
	HEALTH_FLAG_PERMFAIL_MS5611   = (1u << 12),
	HEALTH_FLAG_PERMFAIL_GDK101   = (1u << 13),
	HEALTH_FLAG_PERMFAIL_IMU      = (1u << 14),
} health_flags_t;

// Extended health flags (packed into telemetry reserved bytes; layout stays 80B).
typedef enum {
	HEALTH_EXT_FLAG_CO2_STALE        = (1u << 0),
	HEALTH_EXT_FLAG_PERMFAIL_CO2     = (1u << 1),
	HEALTH_EXT_FLAG_MCP9600_STALE    = (1u << 2),
	HEALTH_EXT_FLAG_PERMFAIL_MCP9600 = (1u << 3),
	HEALTH_EXT_FLAG_GPS_INT_RECENT   = (1u << 4),
	HEALTH_EXT_FLAG_MAG_STALE        = (1u << 5),
	HEALTH_EXT_FLAG_PERMFAIL_MAG     = (1u << 6),
	HEALTH_EXT_FLAG_OZONE_STALE      = (1u << 7),
	HEALTH_EXT_FLAG_PERMFAIL_OZONE   = (1u << 8),
} health_ext_flags_t;

void health_monitor_service_init(void);
void health_monitor_service_tick(uint32_t now_ms);

uint16_t health_monitor_get_flags(void);

uint16_t health_monitor_get_ext_flags(void);

#ifdef __cplusplus
}
#endif
