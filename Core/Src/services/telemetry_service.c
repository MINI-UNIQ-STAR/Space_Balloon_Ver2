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
#include "services/ms5611_service.h"
#include "services/sht31_service.h"
#include "services/telemetry_frame.h"

#include "stm32g4xx_hal.h"

static uint16_t s_seq = 0;
static uint32_t s_next_send_ms = 0;

static uint32_t s_last_pps_seq_seen = 0;
static uint32_t s_epoch_start_ms = 0;
static uint32_t s_epoch_slot = 0;

void telemetry_service_init(void)
{
	s_seq = 0;
	s_next_send_ms = HAL_GetTick();
	pps_capture_init();
	s_last_pps_seq_seen = 0;
	s_epoch_start_ms = 0;
	s_epoch_slot = 0;
}

void telemetry_service_tick(uint32_t now_ms)
{
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

	telemetry_payload_sensor_snapshot_t payload = {0};
	payload.uptime_ms = now_ms;
	payload.status_flags = health_monitor_get_flags();
	{
		const uint16_t ext = health_monitor_get_ext_flags();
		payload.reserved2 = (uint8_t)(ext & 0xFFu);
		payload.reserved3 = (uint8_t)((ext >> 8) & 0xFFu);
	}
	payload.temp_c_x100 = 0;
	payload.sht31_temp_c_x100 = 0;
	payload.sht31_rh_x100 = 0;
	payload.ms5611_press_pa = 0;
	payload.ms5611_temp_c_x100 = 0;
	payload.ms5611_alt_m = 0;
	payload.co2_ppm = 0;
	payload.gdk101_usvh_x100 = 0;
	for (int i = 0; i < 3; i++) {
		payload.accel_mps2_x1000[i] = 0;
		payload.gyro_rads_x1000[i] = 0;
	}

	int32_t accel[3];
	if (imu_service_get_accel_mps2_x1000(accel)) {
		for (int i = 0; i < 3; i++) {
			payload.accel_mps2_x1000[i] = accel[i];
		}
	}
	int32_t gyro[3];
	if (imu_service_get_gyro_rads_x1000(gyro)) {
		for (int i = 0; i < 3; i++) {
			payload.gyro_rads_x1000[i] = gyro[i];
		}
	}

	int16_t t_int;
	if (aux_sensors_get_temp_int_c_x100(&t_int)) {
		payload.temp_c_x100 = t_int;
	}
	uint16_t bat_mv;
	if (aux_sensors_get_bat_mv(&bat_mv)) {
		payload.bat_mv = bat_mv;
	}

	uint16_t co2_ppm;
	if (co2_service_get_ppm(&co2_ppm)) {
		payload.co2_ppm = co2_ppm;
	}

	uint16_t rad_x100;
	if (gdk101_service_get_last_usvh_x100(&rad_x100)) {
		payload.gdk101_usvh_x100 = rad_x100;
	}

	pms_reading_t pm;
	if (air_quality_get_pm(&pm) && pm.valid) {
		payload.pm1_ugm3 = pm.pm1_ugm3;
		payload.pm25_ugm3 = pm.pm25_ugm3;
		payload.pm10_ugm3 = pm.pm10_ugm3;
	}

	nmea_gps_state_t gps;
	if (gps_service_get_state(&gps)) {
		payload.gps_fix = gps.has_fix ? 1u : 0u;
		payload.gps_lat_deg_e7 = gps.lat_deg_e7;
		payload.gps_lon_deg_e7 = gps.lon_deg_e7;
		payload.gps_alt_mm = gps.alt_mm;
		payload.gps_sats_used = gps.sats_used;
		payload.gps_sats_in_view_total = gps.sats_in_view_total;
		payload.gps_sats_in_view_gps = gps.sats_in_view_gps;
		payload.gps_sats_in_view_glonass = gps.sats_in_view_glonass;
		payload.gps_sats_in_view_galileo = gps.sats_in_view_galileo;
		payload.gps_sats_in_view_beidou = gps.sats_in_view_beidou;
	}

	int16_t t_ext;
	uint16_t rh;
	if (sht31_service_get_last(&t_ext, &rh)) {
		payload.sht31_temp_c_x100 = t_ext;
		payload.sht31_rh_x100 = rh;
	}

	int32_t t_baro;
	uint32_t p_baro;
	int32_t alt_m;
	if (ms5611_service_get_last(&t_baro, &p_baro, &alt_m)) {
		payload.ms5611_temp_c_x100 = (int16_t)t_baro;
		payload.ms5611_press_pa = p_baro;
		payload.ms5611_alt_m = alt_m;
	}

	uint8_t frame[96];

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

	(void)uart3_tx_write(frame, n, 10);
}
