#ifndef __TELEMETRY_H
#define __TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#pragma pack(push, 1)

typedef struct {
    // 1. System Status
    uint32_t uptime_ms;
    uint16_t status_flags;
    uint16_t co2_ppm;           // CM1107N

    // 2. IMU (LSM6DSV16X) (x1000 scaled)
    int32_t accel_mps2_x1000[3];
    int32_t gyro_rads_x1000[3];

    // 3. Magnetometer (MLX90393)
    float mag_uT[3];

    // 4. Temperature (x100 scaled)
    int16_t board_temp_c_x100;      // DS18B20 (Board)
    int16_t external_temp_c_x100;   // MCP9600
    int16_t sht31_temp_c_x100;      // SHT31

    // 11. Reserved / Extended Status
    int16_t bat_temp_c_x100;        // DS18B20 (Battery)

    // 5. GPS (XA1110) (x10^7 scaled for lat/lon)
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
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t reserved4;

    uint16_t bat_mv;            // ADC PA1

    // 6. Air Quality
    uint16_t pm1_ugm3;          // PMS3003
    uint16_t pm25_ugm3;
    uint16_t pm10_ugm3;
    int16_t ozone_ppb;          // SEN0321

    // 7. Pressure / Humidity
    uint16_t sht31_rh_x100;     // SHT31
    uint32_t ms5611_press_pa;   // MS5611
    int16_t ms5611_temp_c_x100;

    // 8. Radiation
    uint16_t gdk101_usvh_x100;  // GDK101


    // 9. Heater Status
    uint8_t heater_bat_duty_percent;
    uint8_t heater_board_duty_percent;

    // 10. Altitude Fusion
    float press_alt_m;
    float kf_alt_m;
    float kf_roll_deg;
    float kf_pitch_deg;

} telemetry_payload_sensor_snapshot_t;

typedef struct {
    uint8_t magic[2];      // {0xA5, 0x5A}
    uint8_t version;       // 1
    uint8_t msg_type;      // 0x01 heartbeat, 0x02 sensor snapshot
    uint16_t payload_len;  // bytes
    uint16_t seq;
    uint32_t timestamp_ms;
    telemetry_payload_sensor_snapshot_t payload;
    uint16_t crc16;        // CRC-16/CCITT-FALSE over header+payload
} telemetry_frame_t;

#pragma pack(pop)

// Utils
uint16_t CRC16_CCITT(uint8_t *data, uint16_t length);
void Telemetry_Send(telemetry_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __TELEMETRY_H */
