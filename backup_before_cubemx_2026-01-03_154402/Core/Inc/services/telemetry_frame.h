#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Binary framing for ESP32 receiver.
// Layout (little-endian for multibyte fields):
// [0..1]  magic = 0xA5 0x5A
// [2]     version = 1
// [3]     msg_type
// [4..5]  payload_len (bytes)
// [6..7]  seq
// [8..11] timestamp_ms (HAL_GetTick())
// [12..]  payload (payload_len bytes)
// [..]    crc16_ccitt_false over header+payload (from magic to end of payload)
// CRC appended little-endian (lo, hi)

enum {
	TELEMETRY_MAGIC0 = 0xA5,
	TELEMETRY_MAGIC1 = 0x5A,
	TELEMETRY_VERSION = 1,
};

typedef enum {
	TELEM_MSG_HEARTBEAT = 0x01,
	TELEM_MSG_SENSOR_SNAPSHOT = 0x02,
} telemetry_msg_type_t;

// Fixed payload layouts for stable ESP32 parsing.
// All multibyte fields are little-endian on the wire because we memcpy the struct.
// NOTE: Do not reorder fields without bumping protocol version.

typedef struct {
	uint32_t uptime_ms;
	uint16_t status_flags;
	uint16_t reserved0;
} telemetry_payload_heartbeat_t;


// SI units with fixed-point scaling (integers) to avoid float/ABI pitfalls:
// - accel_mps2_x1000: m/s^2 * 1000
// - gyro_rads_x1000:  rad/s * 1000
// - temp_c_x100:      degC * 100
typedef struct {
	uint32_t uptime_ms;
	uint16_t status_flags;
	uint16_t co2_ppm;
	int32_t accel_mps2_x1000[3];
	int32_t gyro_rads_x1000[3];
	int16_t temp_c_x100;
	int16_t sht31_temp_c_x100;
	int32_t gps_lat_deg_e7;
	int32_t gps_lon_deg_e7;
	int32_t gps_alt_mm;
	uint8_t gps_fix;
	uint8_t gps_sats_used;
	uint8_t gps_sats_in_view_total;
	uint8_t gps_sats_in_view_gps;
	uint8_t gps_sats_in_view_glonass;
	uint8_t gps_sats_in_view_galileo;
	uint8_t gps_sats_in_view_beidou;
	uint8_t reserved2;
	uint8_t reserved3;
	uint16_t bat_mv;
	uint16_t pm1_ugm3;
	uint16_t pm25_ugm3;
	uint16_t pm10_ugm3;
	uint16_t sht31_rh_x100;
	uint32_t ms5611_press_pa;
	int16_t ms5611_temp_c_x100;
	uint16_t gdk101_usvh_x100;
	int32_t ms5611_alt_m;
} telemetry_payload_sensor_snapshot_t;

// Compile-time checks (keep payload stable for ESP32 parsing)
typedef char telemetry_payload_heartbeat_size_check[(sizeof(telemetry_payload_heartbeat_t) == 8u) ? 1 : -1];
typedef char telemetry_payload_snapshot_size_check[(sizeof(telemetry_payload_sensor_snapshot_t) == 80u) ? 1 : -1];

// Returns total frame size written to out, or 0 on failure.
size_t telemetry_build_frame(uint8_t msg_type,
						 const uint8_t *payload,
						 size_t payload_len,
						 uint16_t seq,
						 uint32_t timestamp_ms,
						 uint8_t *out,
						 size_t out_cap);

uint16_t telemetry_crc16_ccitt_false(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
