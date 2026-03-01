/**
 * @file sensors_zephyr.c
 * @brief 센서 드라이버 - Zephyr 내장 드라이버 사용
 * 
 * 지원 센서:
 *   - LSM6DSV16X (IMU): Zephyr 내장
 *   - MS5607 (기압): Zephyr 내장 (MS5611 호환)
 *   - SHT3x (온습도): Zephyr 내장 (shtcx)
 *   - MCP9600 (열전대): Zephyr 내장
 *   - DS18B20 (온도): Zephyr 내장
 *   - GPS: Zephyr GNSS NMEA
 *   - PMS7003 (미세먼지): Zephyr 내장 (PMS3003 호환)
 * 
 * 직접 구현 필요:
 *   - MLX90393 (자기계)
 *   - GDK101 (방사선)
 *   - CM1107N (CO2)
 *   - SEN0321 (오존)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/logging/log.h>
#include "fdir_zephyr.h"

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);

/* ========================================================================== */
/* 센서 디바이스 노드                                                          */
/* ========================================================================== */

/* I2C1 센서 */
#define LSM6DSV16X_NODE DT_NODELABEL(lsm6dsv16x)
#define MLX90393_NODE   DT_NODELABEL(mlx90393)
#define GDK101_NODE     DT_NODELABEL(gdk101)

/* I2C3 센서 */
#define MS5607_NODE     DT_NODELABEL(ms5607)
#define SHT3X_NODE      DT_NODELABEL(sht3x)
#define MCP9600_NODE    DT_NODELABEL(mcp9600)
#define CM1107N_NODE    DT_NODELABEL(cm1107n)
#define SEN0321_NODE    DT_NODELABEL(sen0321)

/* UART 센서 */
#define GPS_NODE        DT_NODELABEL(gps)
#define PMS_NODE        DT_NODELABEL(pms)

/* 1-Wire 센서 */
#define DS18B20_NODE    DT_NODELABEL(ds18b20)

/* 센서 디바이스 포인터 */
static const struct device *imu_dev = NULL;
static const struct device *baro_dev = NULL;
static const struct device *humid_dev = NULL;
static const struct device *thermo_dev = NULL;
static const struct device *gps_dev = NULL;
static const struct device *pms_dev = NULL;
static const struct device *temp_dev = NULL;

/* ========================================================================== */
/* 센서 데이터 버퍼                                                            */
/* ========================================================================== */

static struct {
    int32_t accel[3];
    int32_t gyro[3];
    float mag[3];
    uint32_t pressure_pa;
    int16_t temp_c_x100;
    uint16_t humid_rh_x100;
    uint16_t co2_ppm;
    uint16_t radiation_usvh;
    int16_t ozone_ppb;
    int16_t external_temp_c_x100;
} sensor_data;

/* ========================================================================== */
/* 센서 초기화                                                                 */
/* ========================================================================== */

void Sensors_Init(void) {
    LOG_INF("Initializing sensors...");
    
    /* IMU (LSM6DSV16X) */
#if LSM6DSV16X_NODE != DT_INVALID_NODE
    imu_dev = DEVICE_DT_GET(LSM6DSV16X_NODE);
    if (device_is_ready(imu_dev)) {
        LOG_INF("  LSM6DSV16X: OK");
    } else {
        LOG_WRN("  LSM6DSV16X: NOT READY");
        imu_dev = NULL;
    }
#endif

    /* 기압 (MS5607/MS5611) */
#if MS5607_NODE != DT_INVALID_NODE
    baro_dev = DEVICE_DT_GET(MS5607_NODE);
    if (device_is_ready(baro_dev)) {
        LOG_INF("  MS5607: OK");
    } else {
        LOG_WRN("  MS5607: NOT READY");
        baro_dev = NULL;
    }
#endif

    /* 온습도 (SHT3x) */
#if SHT3X_NODE != DT_INVALID_NODE
    humid_dev = DEVICE_DT_GET(SHT3X_NODE);
    if (device_is_ready(humid_dev)) {
        LOG_INF("  SHT3x: OK");
    } else {
        LOG_WRN("  SHT3x: NOT READY");
        humid_dev = NULL;
    }
#endif

    /* 열전대 (MCP9600) */
#if MCP9600_NODE != DT_INVALID_NODE
    thermo_dev = DEVICE_DT_GET(MCP9600_NODE);
    if (device_is_ready(thermo_dev)) {
        LOG_INF("  MCP9600: OK");
    } else {
        LOG_WRN("  MCP9600: NOT READY");
        thermo_dev = NULL;
    }
#endif

    /* GPS */
#if GPS_NODE != DT_INVALID_NODE
    gps_dev = DEVICE_DT_GET(GPS_NODE);
    if (device_is_ready(gps_dev)) {
        LOG_INF("  GPS: OK");
    } else {
        LOG_WRN("  GPS: NOT READY");
        gps_dev = NULL;
    }
#endif

    /* 미세먼지 (PMS7003/PMS3003) */
#if PMS_NODE != DT_INVALID_NODE
    pms_dev = DEVICE_DT_GET(PMS_NODE);
    if (device_is_ready(pms_dev)) {
        LOG_INF("  PMS: OK");
    } else {
        LOG_WRN("  PMS: NOT READY");
        pms_dev = NULL;
    }
#endif

    /* DS18B20 */
#if DS18B20_NODE != DT_INVALID_NODE
    temp_dev = DEVICE_DT_GET(DS18B20_NODE);
    if (device_is_ready(temp_dev)) {
        LOG_INF("  DS18B20: OK");
    } else {
        LOG_WRN("  DS18B20: NOT READY");
        temp_dev = NULL;
    }
#endif

    LOG_INF("Sensors initialized");
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
    if (imu_dev != NULL) {
        struct sensor_value val[3];
        
        sensor_sample_fetch(imu_dev);
        
        sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, val);
        accel[0] = sensor_value_to_millis(&val[0]);  /* m/s² × 1000 */
        accel[1] = sensor_value_to_millis(&val[1]);
        accel[2] = sensor_value_to_millis(&val[2]);
        
        sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, val);
        gyro[0] = sensor_value_to_millis(&val[0]);   /* rad/s × 1000 */
        gyro[1] = sensor_value_to_millis(&val[1]);
        gyro[2] = sensor_value_to_millis(&val[2]);
    } else {
        /* 시뮬레이션 데이터 */
        accel[0] = 100; accel[1] = 50; accel[2] = 9800;
        gyro[0] = gyro[1] = gyro[2] = 0;
    }
}

void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
    if (baro_dev != NULL) {
        struct sensor_value press, temp;
        
        sensor_sample_fetch(baro_dev);
        sensor_channel_get(baro_dev, SENSOR_CHAN_PRESS, &press);
        sensor_channel_get(baro_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        
        *press_pa = (uint32_t)sensor_value_to_millis(&press);  /* Pa */
        *temp_c_x100 = (int16_t)sensor_value_to_millis(&temp); /* °C × 100 */
    } else {
        *press_pa = 101325;
        *temp_c_x100 = 2500;
    }
}

void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100) {
    if (humid_dev != NULL) {
        struct sensor_value temp, rh;
        
        sensor_sample_fetch(humid_dev);
        sensor_channel_get(humid_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(humid_dev, SENSOR_CHAN_HUMIDITY, &rh);
        
        *temp_c_x100 = (int16_t)sensor_value_to_millis(&temp);
        *rh_x100 = (uint16_t)sensor_value_to_millis(&rh);
    } else {
        *temp_c_x100 = 2500;
        *rh_x100 = 5000;
    }
}

void Sensors_Read_External(int16_t *temp_c_x100) {
    if (thermo_dev != NULL) {
        struct sensor_value temp;
        
        sensor_sample_fetch(thermo_dev);
        sensor_channel_get(thermo_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        
        *temp_c_x100 = (int16_t)sensor_value_to_millis(&temp);
    } else {
        *temp_c_x100 = 2500;
    }
}

void Sensors_Read_All(void *data) {
    uint32_t now = k_uptime_get_32();
    uint16_t rh;
    
    Sensors_Read_IMU(sensor_data.accel, sensor_data.gyro);
    Sensors_Read_Baro(&sensor_data.pressure_pa, &sensor_data.temp_c_x100);
    Sensors_Read_Humid(&sensor_data.temp_c_x100, &rh);
    Sensors_Read_External(&sensor_data.external_temp_c_x100);
    
    /* FDIR에 정상 보고 */
    FDIR_ReportOK(SENSOR_ID_IMU, now);
    FDIR_ReportOK(SENSOR_ID_BARO, now);
    FDIR_ReportOK(SENSOR_ID_SHT, now);
}

/* ========================================================================== */
/* 기타 센서 함수                                                              */
/* ========================================================================== */

void Sensors_Read_Mag(float mag[3]) {
    /* MLX90393 - 직접 구현 필요 */
    mag[0] = mag[1] = mag[2] = 0.0f;
}

void Sensors_Read_Rad(uint16_t *uSvh) {
    /* GDK101 - 직접 구현 필요 */
    *uSvh = 0;
}

void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix,
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year) {
    /* GPS - GNSS NMEA */
    *lat = *lon = 0;
    *alt = 0.0f;
    *fix = 0;
    *sats = 0;
}

void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, 
                             uint16_t *pm1_0, uint16_t *pm2_5) {
    /* CM1107N, SEN0321 - 직접 구현 필요 */
    *co2 = 400;
    *ozone = 0;
    
    /* PMS7003/PMS3003 */
    if (pms_dev != NULL) {
        struct sensor_value pm1, pm25;
        sensor_sample_fetch(pms_dev);
        sensor_channel_get(pms_dev, SENSOR_CHAN_PM_1_0, &pm1);
        sensor_channel_get(pms_dev, SENSOR_CHAN_PM_2_5, &pm25);
        *pm1_0 = (uint16_t)pm1.val1;
        *pm2_5 = (uint16_t)pm25.val1;
    } else {
        *pm1_0 = *pm2_5 = 0;
    }
}

void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
    *mv = 3700;
    *temp_c_x100 = 2500;
}

void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
    if (temp_dev != NULL) {
        struct sensor_value temp;
        sensor_sample_fetch(temp_dev);
        sensor_channel_get(temp_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        *temp_c_x100 = (int16_t)sensor_value_to_millis(&temp);
    } else {
        *temp_c_x100 = 2500;
    }
}

void Sensors_SetHeater_SHT31(uint8_t enable) {
    (void)enable;
}
