/**
 * @file telemetry_zephyr.c
 * @brief 텔레메트리 - Zephyr RTOS 포팅 (UART3 148바이트 프레임)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "fdir_zephyr.h"

LOG_MODULE_REGISTER(telemetry, LOG_LEVEL_INF);

/* ========================================================================== */
/* 프레임 구조 정의                                                            */
/* ========================================================================== */

#define TELEMETRY_MAGIC_0    0xA5
#define TELEMETRY_MAGIC_1    0x5A
#define TELEMETRY_VERSION    1
#define TELEMETRY_MSG_TYPE   0x02  /* Sensor Snapshot */
#define TELEMETRY_PAYLOAD_LEN 132
#define TELEMETRY_FRAME_SIZE  148

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

typedef struct {
    uint8_t magic[2];
    uint8_t version;
    uint8_t msg_type;
    uint16_t payload_len;
    uint16_t seq;
    uint32_t timestamp_ms;
    telemetry_payload_t payload;
    uint16_t crc16;
} telemetry_frame_t;

#pragma pack(pop)

/* ========================================================================== */
/* 전역 변수                                                                   */
/* ========================================================================== */

static uint16_t seq_number = 0;
static telemetry_frame_t tx_frame;

/* ========================================================================== */
/* UART 장치 (실제 하드웨어용)                                                  */
/* ========================================================================== */

#if defined(CONFIG_BOARD_WEARCT_STM32G431_CORE) || defined(CONFIG_BOARD_NUCLEO_G431RB)
#define UART_DEV_NODE DT_NODELABEL(usart3)
#else
/* QEMU 시뮬레이션 */
#define UART_DEV_NODE DT_INVALID_NODE
#endif

static const struct device *uart_dev = NULL;
static bool uart_ready = false;

/* ========================================================================== */
/* CRC-16/CCITT-FALSE                                                         */
/* ========================================================================== */

static uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    
    return crc;
}

/* ========================================================================== */
/* 초기화                                                                      */
/* ========================================================================== */

void Telemetry_Init(void) {
#if UART_DEV_NODE != DT_INVALID_NODE
    uart_dev = DEVICE_DT_GET(UART_DEV_NODE);
    if (device_is_ready(uart_dev)) {
        uart_ready = true;
        LOG_INF("Telemetry initialized (UART3, 148 bytes/frame, BINARY)");
    } else {
        LOG_ERR("UART3 not ready");
    }
#else
    LOG_INF("Telemetry initialized (QEMU simulation mode)");
#endif
}

/* ========================================================================== */
/* 프레임 생성                                                                 */
/* ========================================================================== */

void Telemetry_BuildFrame(uint16_t status_flags, uint32_t uptime_ms,
                          int32_t accel[3], int32_t gyro[3],
                          uint32_t press_pa, int16_t temp_c_x100,
                          float alt_m) {
    /* 헤더 */
    tx_frame.magic[0] = TELEMETRY_MAGIC_0;
    tx_frame.magic[1] = TELEMETRY_MAGIC_1;
    tx_frame.version = TELEMETRY_VERSION;
    tx_frame.msg_type = TELEMETRY_MSG_TYPE;
    tx_frame.payload_len = TELEMETRY_PAYLOAD_LEN;
    tx_frame.seq = seq_number++;
    tx_frame.timestamp_ms = uptime_ms;
    
    /* 페이로드 - 실제 센서 데이터 사용 */
    memset(&tx_frame.payload, 0, sizeof(telemetry_payload_t));
    tx_frame.payload.uptime_ms = uptime_ms;
    tx_frame.payload.status_flags = status_flags;
    
    /* IMU 데이터 */
    if (accel) {
        tx_frame.payload.accel_mps2_x1000[0] = accel[0];
        tx_frame.payload.accel_mps2_x1000[1] = accel[1];
        tx_frame.payload.accel_mps2_x1000[2] = accel[2];
    }
    
    if (gyro) {
        tx_frame.payload.gyro_rads_x1000[0] = gyro[0];
        tx_frame.payload.gyro_rads_x1000[1] = gyro[1];
        tx_frame.payload.gyro_rads_x1000[2] = gyro[2];
    }
    
    /* 기압/고도 데이터 */
    tx_frame.payload.ms5611_press_pa = press_pa;
    tx_frame.payload.ms5611_temp_c_x100 = temp_c_x100;
    tx_frame.payload.sht31_temp_c_x100 = temp_c_x100;
    tx_frame.payload.press_alt_m = alt_m;
    tx_frame.payload.kf_alt_m = alt_m;
    
    /* 배터리 (센서에서 읽어야 하지만 기본값) */
    tx_frame.payload.bat_mv = 3700;
    tx_frame.payload.bat_temp_c_x100 = temp_c_x100;  /* 실제 온도 사용 */
    tx_frame.payload.board_temp_c_x100 = temp_c_x100;
    
    /* CRC 계산 (헤더 + 페이로드, 144바이트) */
    tx_frame.crc16 = crc16_ccitt((const uint8_t *)&tx_frame, 144);
}

/* ========================================================================== */
/* 프레임 전송                                                                 */
/* ========================================================================== */

void Telemetry_Send(void) {
#if UART_DEV_NODE != DT_INVALID_NODE
    /* 실제 하드웨어: UART3로 148바이트 바이너리 전송 */
    if (uart_ready && uart_dev != NULL) {
        /* UART 폴링 전송 */
        for (int i = 0; i < TELEMETRY_FRAME_SIZE; i++) {
            uart_poll_out(uart_dev, ((uint8_t*)&tx_frame)[i]);
        }
        
        /* 디버그 로그 (1초마다) */
        if (tx_frame.seq % 50 == 0) {
            LOG_INF("TX: seq=%u, flags=0x%04X, crc=0x%04X",
                    tx_frame.seq, tx_frame.payload.status_flags, tx_frame.crc16);
        }
    }
#else
    /* QEMU 시뮬레이션: 로그만 출력 */
    LOG_INF("TELEMETRY FRAME: seq=%u, uptime=%ums, flags=0x%04X, press=%uPa, alt=%.1fm",
            tx_frame.seq, tx_frame.timestamp_ms, 
            tx_frame.payload.status_flags,
            tx_frame.payload.ms5611_press_pa,
            tx_frame.payload.press_alt_m);
#endif
}

/* ========================================================================== */
/* 전체 텔레메트리 처리                                                          */
/* ========================================================================== */

void Telemetry_Process(uint16_t status_flags, uint32_t uptime_ms,
                       int32_t accel[3], int32_t gyro[3],
                       uint32_t press_pa, int16_t temp_c_x100,
                       float alt_m) {
    Telemetry_BuildFrame(status_flags, uptime_ms, accel, gyro, press_pa, temp_c_x100, alt_m);
    Telemetry_Send();
}
