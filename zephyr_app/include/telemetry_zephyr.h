#ifndef TELEMETRY_ZEPHYR_H
#define TELEMETRY_ZEPHYR_H

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
  uint32_t uptime_ms;
  uint16_t status_flags;
  uint16_t co2_ppm;
  int32_t accel_mps2_x1000[3];
  int32_t gyro_rads_x1000[3];
  float mag_uT[3];
  int16_t board_temp_c_x100;
  int16_t external_temp_c_x100;
  int16_t sht31_temp_c_x100;
  int16_t bat_temp_c_x100;
  int32_t gps_lat_deg_e7;
  int32_t gps_lon_deg_e7;
  float gps_alt_m;
  uint8_t gps_fix;
  uint8_t gps_sats_used;
  uint8_t gps_sats_in_view_total;
  uint8_t gps_sats_in_view_gps;
  uint8_t gps_sats_in_view_glonass;
  uint8_t gps_sats_in_view_galileo;
  uint8_t gps_sats_in_view_beidou;
  uint8_t gps_utc_hour;
  uint8_t gps_utc_min;
  uint8_t gps_utc_sec;
  uint8_t gps_utc_day;
  uint8_t gps_utc_month;
  uint16_t gps_utc_year;
  uint16_t bat_mv;
  uint16_t pm1_ugm3;
  uint16_t pm25_ugm3;
  uint16_t pm10_ugm3;
  int16_t ozone_ppb;
  uint16_t sht31_rh_x100;
  uint32_t ms5611_press_pa;
  int16_t ms5611_temp_c_x100;
  uint16_t gdk101_usvh_x100;
  uint8_t heater_bat_duty_percent;
  uint8_t heater_board_duty_percent;
  float press_alt_m;
  float kf_alt_m;
  float kf_roll_deg;
  float kf_pitch_deg;
} telemetry_payload_t;

#pragma pack(pop)

void Telemetry_Init(void);
void Telemetry_Process(telemetry_payload_t *payload);

#endif /* TELEMETRY_ZEPHYR_H */
