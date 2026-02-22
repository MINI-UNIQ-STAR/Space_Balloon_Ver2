/**
 * @file sensors_zephyr.c
 * @brief 센서 드라이버 - Zephyr 포팅 (시뮬레이션용)
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "fdir_zephyr.h"

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);

/* ========================================================================== */
/* 센서 데이터 버퍼                                                            */
/* ========================================================================== */

static struct {
    int32_t accel[3];
    int32_t gyro[3];
    uint32_t pressure_pa;
    int16_t temp_c_x100;
    uint16_t humid_rh_x100;
    uint16_t co2_ppm;
    uint16_t radiation_usvh;
} sensor_data;

/* ========================================================================== */
/* 센서 초기화                                                                 */
/* ========================================================================== */

void Sensors_Init(void) {
    LOG_INF("Sensors initialized (simulation mode)");
}

void Sensors_Reset(int sensor_id) {
    LOG_WRN("Sensor %d reset requested", sensor_id);
}

void Sensors_ProcessReset(void) {
    /* Non-blocking reset state machine */
}

/* ========================================================================== */
/* 센서 데이터 읽기                                                            */
/* ========================================================================== */

void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]) {
    /* 시뮬레이션 데이터 */
    accel[0] = 100;
    accel[1] = 50;
    accel[2] = 9800;  /* 1g */
    gyro[0] = gyro[1] = gyro[2] = 0;
}

void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
    *press_pa = 101325;
    *temp_c_x100 = 2500;
}

void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100) {
    *temp_c_x100 = 2500;
    *rh_x100 = 5000;
}

void Sensors_Read_All(void *data) {
    uint32_t now = k_uptime_get_32();
    uint16_t rh;
    
    Sensors_Read_IMU(sensor_data.accel, sensor_data.gyro);
    Sensors_Read_Baro(&sensor_data.pressure_pa, &sensor_data.temp_c_x100);
    Sensors_Read_Humid(&sensor_data.temp_c_x100, &rh);
    
    /* FDIR에 정상 보고 */
    FDIR_ReportOK(SENSOR_ID_IMU, now);
    FDIR_ReportOK(SENSOR_ID_BARO, now);
    FDIR_ReportOK(SENSOR_ID_SHT, now);
}

/* ========================================================================== */
/* 기타 센서 함수 (스텁)                                                       */
/* ========================================================================== */

void Sensors_Read_Mag(float mag[3]) {
    mag[0] = mag[1] = mag[2] = 0.0f;
}

void Sensors_Read_Rad(uint16_t *uSvh) {
    *uSvh = 0;
}

void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix,
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year) {
    *lat = *lon = 0;
    *alt = 0.0f;
    *fix = 0;
    *sats = 0;
}

void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, 
                             uint16_t *pm1_0, uint16_t *pm2_5) {
    *co2 = 400;
    *ozone = 0;
    *pm1_0 = *pm2_5 = 0;
}

void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
    *mv = 3700;
    *temp_c_x100 = 2500;
}

void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
    *temp_c_x100 = 2500;
}

void Sensors_Read_External(int16_t *temp_c_x100) {
    *temp_c_x100 = 2500;
}

void Sensors_SetHeater_SHT31(uint8_t enable) {
    (void)enable;
}
