#include "services/telemetry_service.h"

#include "drivers/uart_tx.h"
#include "drivers/pps_capture.h"
#include "services/aux_sensors_service.h"
#include "services/air_quality_service.h"
#include "services/co2_service.h"
#include "services/gps_service.h"
#include "services/gdk101_service.h"
#include "services/health_monitor_service.h"
#include "services/imu_service.h"
#include "services/mag_service.h"
#include "services/mcp9600_service.h"
#include "services/heater_service.h"
#include "services/ozone_service.h"
#include "services/alt_kf_service.h"
#include "services/ms5611_service.h"
#include "services/sht31_service.h"
#include "services/telemetry_frame.h"
#include "services/telemetry_payload_store.h"

#include "stm32g4xx_hal.h"

static uint16_t s_seq = 0;
static uint32_t s_next_send_ms = 0;

static uint32_t s_last_pps_seq_seen = 0;
static uint32_t s_epoch_start_ms = 0;
static uint32_t s_epoch_slot = 0;

static bool s_last_payload_valid = false;
static telemetry_payload_sensor_snapshot_t s_last_payload;

// Telemetry service execution time for observability (no protocol size change).
// Stored in payload.reserved4 as 100us units (0..255 => 0..25.5ms), last measured value.
static uint8_t s_last_telem_exec_100us = 0;

void telemetry_service_init(void)
{
	s_seq = 0;
	s_next_send_ms = HAL_GetTick();
	pps_capture_init();
	s_last_pps_seq_seen = 0;
	s_epoch_start_ms = 0;
	s_epoch_slot = 0;
	s_last_payload_valid = false;
	s_last_telem_exec_100us = 0;
}

void telemetry_service_tick(uint32_t now_ms)
{
	const uint32_t cycles_per_us = (uint32_t)(SystemCoreClock / 1000000u);
	const uint32_t start_cycles = DWT->CYCCNT;

	// PPS discipline: when GPS 1PPS is available, align 50Hz schedule to the PPS second boundary.
	{
		uint32_t pps_ms;
		uint32_t pps_seq;
		if (pps_capture_get_last(&pps_ms, &pps_seq) && pps_seq != s_last_pps_seq_seen) {
			s_last_pps_seq_seen = pps_seq;
			s_epoch_start_ms = pps_ms;
			s_epoch_slot = 0;
			s_next_send_ms = s_epoch_start_ms;
		}
	}

	// MVP: fixed-struct snapshot at 50 Hz (ESP32 receiver).
	if ((int32_t)(now_ms - s_next_send_ms) < 0) {
		return;
	}

	if (s_epoch_start_ms != 0u) {
		// If we're disciplined, compute next slot within the current PPS second.
		// 50 slots * 20ms = 1000ms.
		uint32_t slot;
		if (now_ms <= s_epoch_start_ms) {
			slot = 0;
		} else {
			slot = (now_ms - s_epoch_start_ms) / 20u;
			if (slot > 49u) {
				// We're beyond the current PPS second; wait for the next PPS edge.
				s_next_send_ms = now_ms + 20u;
				s_epoch_start_ms = 0u;
				s_epoch_slot = 0u;
			} else {
				// Advance to the next slot boundary.
				s_epoch_slot = slot + 1u;
				s_next_send_ms = s_epoch_start_ms + (s_epoch_slot * 20u);
			}
		}
	} else {
		// Fallback: free-running schedule.
		s_next_send_ms = now_ms + 20u;
	}

	// Read a consistent snapshot from the shared payload store.
	// Never block the realtime sender for long; if we can't lock, reuse last payload.
	telemetry_payload_sensor_snapshot_t payload;
	if (telemetry_payload_store_read_copy(&payload, 0u)) {
		s_last_payload = payload;
		s_last_payload_valid = true;
	} else if (s_last_payload_valid) {
		payload = s_last_payload;
	} else {
		// On startup before writers run.
		payload = (telemetry_payload_sensor_snapshot_t){0};
		payload.uptime_ms = now_ms;
	}

	// Embed observability (no protocol size change).
	// `reserved4` carries last telemetry_service_tick execution time in 100us units.
	payload.reserved4 = s_last_telem_exec_100us;

	uint8_t frame[128];

	const size_t n = telemetry_build_frame(
			(uint8_t)TELEM_MSG_SENSOR_SNAPSHOT,
			(const uint8_t *)&payload,
			sizeof(payload),
			s_seq++,
			now_ms,
			frame,
			sizeof(frame));
	if (n == 0u) {
		return;
	}

	// Never block the 50Hz realtime path on UART I/O.
	(void)uart3_tx_write(frame, n, 0u);

	// Measure and store exec time for the *next* frame.
	if (cycles_per_us != 0u) {
		uint32_t exec_us = (DWT->CYCCNT - start_cycles) / cycles_per_us;
		// Round to nearest 100us.
		uint32_t exec_100us = (exec_us + 50u) / 100u;
		if (exec_100us > 255u) {
			exec_100us = 255u;
		}
		s_last_telem_exec_100us = (uint8_t)exec_100us;
	}
}
