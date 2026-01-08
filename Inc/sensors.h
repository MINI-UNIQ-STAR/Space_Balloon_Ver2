#ifndef __SENSORS_H
#define __SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "telemetry.h"
#include "main.h" // For HAL includes if available

// Simple status return for sensor ops
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR = 1,
    SENSOR_TIMEOUT = 2
} SensorStatus_t;

// Sensor IDs for FDIR
typedef enum {
    SENSOR_ID_IMU = 0,
    SENSOR_ID_MAG,
    SENSOR_ID_BARO,
    SENSOR_ID_GPS,
    SENSOR_ID_PMS,
    SENSOR_ID_CO2,
    SENSOR_ID_SHT,
    SENSOR_ID_RAD,
    SENSOR_ID_EXT_TEMP,
    SENSOR_ID_COUNT
} SensorID_t;

// Initialization & Recovery
void Sensors_Init(void);
void Sensors_Reset(SensorID_t id); // FDIR Recovery
void Sensors_Init_I2C1(void); // Downside
void Sensors_Init_I2C3(void); // Upside
void Sensors_Init_UART(void); // GPS, LoRa, PM

// Data Acquisition
SensorStatus_t Sensors_Read_All(telemetry_payload_sensor_snapshot_t *data);

// Individual Read Wrappers (Mock/Real)
// Downside
void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]);
void Sensors_Read_Mag(float mag[3]);
void Sensors_Read_Rad(uint16_t *uSvh);
void Sensors_Read_SFLP(float quaternion[4]); // x, y, z, w

// Upside
void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100);
void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100);
void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5);
void Sensors_SetHeater_SHT31(uint8_t enable);

// ADC/OneWire/Thermocouple
void Sensors_Init_1Wire(void); // DS18B20 initialization
void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100);
void Sensors_Read_BoardTemp(int16_t *temp_c_x100);
void Sensors_Read_External(int16_t *temp_c_x100);

// GPS
// Passing pointers to fill telemetry fields directly is easiest, or struct
void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix,
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year);

#ifdef __cplusplus
}
#endif

#endif /* __SENSORS_H */
