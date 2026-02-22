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

/* 센서 데이터 */
static struct {
    int32_t accel[3];
    int32_t gyro[3];
    uint32_t pressure_pa;
    int16_t temp_c_x100;
    float altitude_m;
    uint32_t update_count;
} sim_data;

/* 센서 함수 선언 */
extern void Sensors_Init(void);
extern void Sensors_Read_All(void *data);
extern void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]);

/* 텔레메트리 함수 선언 */
extern void Telemetry_Init(void);
extern void Telemetry_Process(uint16_t status_flags, uint32_t uptime_ms,
                              int32_t accel[3], int32_t gyro[3],
                              uint32_t press_pa, int16_t temp_c_x100,
                              float alt_m);

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
    
    uint32_t fault_inject_count = 0;
    
    while (1) {
        uint32_t now = k_uptime_get_32();
        sim_data.update_count++;
        
        /* 실제 센서 데이터 읽기 */
        Sensors_Read_IMU(sim_data.accel, sim_data.gyro);
        
        /* 고도 시뮬레이션 */
        sim_data.altitude_m += 0.1f;
        sim_data.pressure_pa = 101325 - (uint32_t)(sim_data.altitude_m * 12);
        sim_data.temp_c_x100 = 2500 - (int16_t)(sim_data.altitude_m / 100.0f * 650);
        
        /* FDIR에 정상 보고 */
        FDIR_ReportOK(SENSOR_ID_BARO, now);
        FDIR_ReportOK(SENSOR_ID_GPS, now);
        
        /* 온도 보호 테스트 */
        FDIR_CheckTemperatureProtection(sim_data.temp_c_x100);
        
        /* FDIR 범위 검증 */
        FDIR_ValidateRange_Baro(sim_data.pressure_pa);
        
        /* 상태 플래그 확인 */
        uint16_t flags = FDIR_GetStatusFlags();
        
        if (sim_data.update_count % 50 == 0) {
            LOG_INF("Sensors: accel=[%d,%d,%d], press=%" PRIu32 "Pa, alt=%.1fm, flags=0x%04X",
                    sim_data.accel[0], sim_data.accel[1], sim_data.accel[2],
                    sim_data.pressure_pa, sim_data.altitude_m, flags);
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
                          sim_data.accel, sim_data.gyro,
                          sim_data.pressure_pa, sim_data.temp_c_x100,
                          sim_data.altitude_m);
        
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
