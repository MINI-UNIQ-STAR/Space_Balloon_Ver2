/**
 * @file main.c
 * @brief Space Balloon Ver2 - Zephyr RTOS 메인 진입점
 * @author Hyeonsu Park
 * @date 2026-02-22
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "fdir_zephyr.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* 스레드 스택 정의 */
K_THREAD_STACK_DEFINE(fdir_stack, 1024);
K_THREAD_STACK_DEFINE(sensor_stack, 2048);
K_THREAD_STACK_DEFINE(telemetry_stack, 1024);

/* 스레드 ID */
static struct k_thread fdir_thread;
static struct k_thread sensor_thread;
static struct k_thread telemetry_thread;

/* 센서 데이터 - 실제 센서에서 읽은 값 저장 */
static struct {
    int32_t accel[3];
    int32_t gyro[3];
    uint32_t pressure_pa;
    int16_t temp_c_x100;
    uint16_t humid_rh_x100;
    int16_t external_temp_c_x100;
    uint16_t co2_ppm;
    int16_t ozone_ppb;
    uint16_t radiation_usvh_x100;
    float altitude_m;
    uint32_t update_count;
} sensor_data;

/* 센서 함수 선언 */
extern void Sensors_Init(void);
extern void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]);
extern void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100);
extern void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100);
extern void Sensors_Read_External(int16_t *temp_c_x100);
extern void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5);
extern void Sensors_Read_Rad(uint16_t *usvh);
extern void Sensors_Read_BoardTemp(int16_t *temp_c_x100);
extern void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100);

/* 텔레메트리 함수 선언 */
extern void Telemetry_Init(void);
extern void Telemetry_Process(uint16_t status_flags, uint32_t uptime_ms,
                              int32_t accel[3], int32_t gyro[3],
                              uint32_t press_pa, int16_t temp_c_x100,
                              float alt_m);

/* 고도 계산 (기압 기반) */
static float calculate_altitude(uint32_t press_pa) {
    /* 국제 표준 대기 모델 */
    const float P0 = 101325.0f;  /* 해면 기압 (Pa) */
    const float T0 = 288.15f;    /* 해면 온도 (K) */
    const float L = 0.0065f;     /* 기온 감률 (K/m) */
    const float R = 8.31447f;    /* 기체 상수 */
    const float M = 0.0289644f;  /* 공기 몰질량 (kg/mol) */
    const float g = 9.80665f;    /* 중력가속도 */
    
    if (press_pa <= 0) return 0.0f;
    
    float P = (float)press_pa;
    float alt = (T0 / L) * (1.0f - powf(P / P0, (R * L) / (g * M)));
    
    return alt;
}

/**
 * @brief FDIR 스레드 (50Hz)
 */
static void fdir_thread_fn(void *p1, void *p2, void *p3) {
    LOG_INF("FDIR thread started");
    
    while (1) {
        uint32_t now = k_uptime_get_32();
        FDIR_Update(now);
        k_msleep(20);  /* 50 Hz */
    }
}

/**
 * @brief 센서 스레드 (10Hz)
 */
static void sensor_thread_fn(void *p1, void *p2, void *p3) {
    LOG_INF("Sensor thread started");
    
    while (1) {
        uint32_t now = k_uptime_get_32();
        sensor_data.update_count++;
        
        /* 실제 센서에서 데이터 읽기 */
        Sensors_Read_IMU(sensor_data.accel, sensor_data.gyro);
        Sensors_Read_Baro(&sensor_data.pressure_pa, &sensor_data.temp_c_x100);
        Sensors_Read_Humid(&sensor_data.temp_c_x100, &sensor_data.humid_rh_x100);
        Sensors_Read_External(&sensor_data.external_temp_c_x100);
        
        /* 공기질 센서 */
        uint16_t pm1_0, pm2_5;
        Sensors_Read_AirQuality(&sensor_data.co2_ppm, &sensor_data.ozone_ppb, 
                                &pm1_0, &pm2_5);
        
        /* 방사선 센서 */
        Sensors_Read_Rad(&sensor_data.radiation_usvh_x100);
        
        /* 고도 계산 */
        sensor_data.altitude_m = calculate_altitude(sensor_data.pressure_pa);
        
        /* 온도 보호 */
        FDIR_CheckTemperatureProtection(sensor_data.temp_c_x100);
        
        /* FDIR 범위 검증 */
        FDIR_ValidateRange_Baro(sensor_data.pressure_pa);
        
        /* 상태 플래그 확인 */
        uint16_t flags = FDIR_GetStatusFlags();
        
        /* 주기적 로그 (1초마다) */
        if (sensor_data.update_count % 10 == 0) {
            LOG_INF("Sensors: accel=[%d,%d,%d], press=%uPa, alt=%.1fm, temp=%.1fC, flags=0x%04X",
                    sensor_data.accel[0], sensor_data.accel[1], sensor_data.accel[2],
                    sensor_data.pressure_pa, sensor_data.altitude_m, 
                    sensor_data.temp_c_x100 / 100.0f, flags);
        }
        
        k_msleep(100);  /* 10 Hz */
    }
}

/**
 * @brief 텔레메트리 스레드 (50Hz)
 */
static void telemetry_thread_fn(void *p1, void *p2, void *p3) {
    LOG_INF("Telemetry thread started (UART3)");
    
    while (1) {
        uint32_t now = k_uptime_get_32();
        uint16_t flags = FDIR_GetStatusFlags();
        
        /* UART3로 148바이트 프레임 전송 */
        Telemetry_Process(flags, now,
                          sensor_data.accel, sensor_data.gyro,
                          sensor_data.pressure_pa, sensor_data.temp_c_x100,
                          sensor_data.altitude_m);
        
        k_msleep(20);  /* 50 Hz */
    }
}

/**
 * @brief 메인 진입점
 */
int main(void) {
    LOG_INF("========================================");
    LOG_INF("Space Balloon Ver2 - Zephyr RTOS");
    LOG_INF("========================================");
    LOG_INF("Board: %s", CONFIG_BOARD);
    LOG_INF("Build: " __DATE__ " " __TIME__);
    
    /* 센서 초기화 */
    Sensors_Init();
    
    /* 텔레메트리 초기화 */
    Telemetry_Init();
    
    /* FDIR 초기화 */
    FDIR_Init();
    
    /* 스레드 시작 */
    k_thread_create(&fdir_thread, fdir_stack,
                    K_THREAD_STACK_SIZEOF(fdir_stack),
                    fdir_thread_fn, NULL, NULL, NULL,
                    5, 0, K_NO_WAIT);
    
    k_thread_create(&sensor_thread, sensor_stack,
                    K_THREAD_STACK_SIZEOF(sensor_stack),
                    sensor_thread_fn, NULL, NULL, NULL,
                    4, 0, K_NO_WAIT);
    
    k_thread_create(&telemetry_thread, telemetry_stack,
                    K_THREAD_STACK_SIZEOF(telemetry_stack),
                    telemetry_thread_fn, NULL, NULL, NULL,
                    3, 0, K_NO_WAIT);
    
    LOG_INF("All threads started");
    
    /* 메인 스레드는 모니터링 */
    while (1) {
        k_msleep(5000);
        uint16_t flags = FDIR_GetStatusFlags();
        LOG_INF("=== SYSTEM STATUS: flags=0x%04X ===", flags);
    }
    
    return 0;
}
