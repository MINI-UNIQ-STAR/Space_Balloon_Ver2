/**
 * @file telemetry.h
 * @brief 텔레메트리 프레임 인터페이스 - 센서 데이터 전송
 * @details 132바이트 센서 스냅샷 페이로드 + 16바이트 헤더/CRC
 *          프레임 구조: Magic(2) + Version(1) + MsgType(1) + PayloadLen(2) + Seq(2) + Timestamp(4) + Payload(132) + CRC16(2)
 *          전송: UART3 (115200 baud), 50Hz 주기
 *          CRC: CRC-16/CCITT-FALSE (Poly=0x1021, Init=0xFFFF, XorOut=0x0000)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __TELEMETRY_H
#define __TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#pragma pack(push, 1)

/**
 * @brief 센서 데이터 스냅샷 페이로드 (132 바이트)
 * @details 11개 센서 데이터 통합 패키징
 *          스케일링 규칙:
 *          - 정수 온도: °C × 100 (0.01°C 분해능)
 *          - 정수 가속도/자이로: m/s² × 1000, rad/s × 1000
 *          - GPS 위경도: 도 × 10^7 (0.0000001° 분해능, ~1.1cm)
 *          - float: 부동소수점 직접 전송 (고도, 자기장, 융합 데이터)
 */
typedef struct {
    /* 1. 시스템 상태 (6 bytes) */
    uint32_t uptime_ms;              /**< 시스템 가동 시간 (ms, HAL_GetTick) */
    uint16_t status_flags;           /**< FDIR 상태 플래그 (FDIR_GetStatusFlags) */
    uint16_t co2_ppm;                /**< CO2 농도 (ppm, CM1107N, I2C3 0x31) */

    /* 2. IMU 데이터 (24 bytes, LSM6DSV16X, I2C1 0x6B) */
    int32_t accel_mps2_x1000[3];     /**< 가속도 [x, y, z] (m/s² × 1000, ±16g, 480Hz) */
    int32_t gyro_rads_x1000[3];      /**< 각속도 [x, y, z] (rad/s × 1000, ±2000dps, 480Hz) */

    /* 3. 자기계 데이터 (12 bytes, MLX90393, I2C1 0x0C) */
    float mag_uT[3];                 /**< 자기장 [x, y, z] (µT, 50Hz) */

    /* 4. 온도 센서 (6 bytes) */
    int16_t board_temp_c_x100;       /**< 보드 온도 (°C × 100, DS18B20[1], 1-Wire) */
    int16_t external_temp_c_x100;    /**< 외부 온도 (°C × 100, MCP9600 K-Type 열전대, I2C3 0x60) */
    int16_t sht31_temp_c_x100;       /**< SHT31 온도 (°C × 100, I2C3 0x44) */

    /* 11. 배터리 온도 (2 bytes) */
    int16_t bat_temp_c_x100;         /**< 배터리 온도 (°C × 100, DS18B20[0], PID 제어용) */

    /* 5. GPS 데이터 (32 bytes, XA1110, UART1 115200 baud) */
    int32_t gps_lat_deg_e7;          /**< 위도 (도 × 10^7, -90° ~ +90°) */
    int32_t gps_lon_deg_e7;          /**< 경도 (도 × 10^7, -180° ~ +180°) */
    float gps_alt_m;                 /**< GPS 고도 (m, 해수면 기준) */
    uint8_t gps_fix;                 /**< Fix 상태 (0=No Fix, 1=2D, 2=3D) */
    uint8_t gps_sats_used;           /**< 사용 중인 위성 수 (Fix 계산용) */
    uint8_t gps_sats_in_view_total;  /**< 총 가시 위성 수 (모든 GNSS) */
    uint8_t gps_sats_in_view_gps;    /**< GPS 위성 수 */
    uint8_t gps_sats_in_view_glonass;/**< GLONASS 위성 수 */
    uint8_t gps_sats_in_view_galileo;/**< Galileo 위성 수 */
    uint8_t gps_sats_in_view_beidou; /**< BeiDou 위성 수 */

    /* GPS UTC 시간 (8 bytes, NMEA RMC 문장) */
    uint8_t gps_utc_hour;            /**< UTC 시 (0-23) */
    uint8_t gps_utc_min;             /**< UTC 분 (0-59) */
    uint8_t gps_utc_sec;             /**< UTC 초 (0-59) */
    uint8_t gps_utc_day;             /**< UTC 일 (1-31) */
    uint8_t gps_utc_month;           /**< UTC 월 (1-12) */
    uint16_t gps_utc_year;           /**< UTC 년 (2000-2099) */

    uint16_t bat_mv;                 /**< 배터리 전압 (mV, ADC1_IN2 PA1, 분압비 6:1) */

    /* 6. 공기질 데이터 (8 bytes) */
    uint16_t pm1_ugm3;               /**< PM1.0 (µg/m³, PMS3003, UART2 9600 baud) */
    uint16_t pm25_ugm3;              /**< PM2.5 (µg/m³, PMS3003) */
    uint16_t pm10_ugm3;              /**< PM10 (µg/m³, PMS3003) */
    int16_t ozone_ppb;               /**< 오존 농도 (ppb, SEN0321, I2C3 0x70) */

    /* 7. 기압/습도 데이터 (8 bytes) */
    uint16_t sht31_rh_x100;          /**< 상대습도 (%RH × 100, SHT31, I2C3 0x44) */
    uint32_t ms5611_press_pa;        /**< 기압 (Pa, MS5611, I2C3 0x77) */
    int16_t ms5611_temp_c_x100;      /**< MS5611 온도 (°C × 100) */

    /* 8. 방사선 데이터 (2 bytes) */
    uint16_t gdk101_usvh_x100;       /**< 방사선 선량율 (µSv/h × 100, GDK101, I2C1 0x18, 1분 평균) */

    /* 9. 히터 제어 상태 (2 bytes) */
    uint8_t heater_bat_duty_percent; /**< 배터리 히터 듀티 사이클 (0-60%, PID 제어) */
    uint8_t heater_board_duty_percent;/**< 보드 히터 듀티 사이클 (0-100%, PID 제어) */

    /* 10. 고도 융합 및 자세 데이터 (16 bytes) */
    float press_alt_m;               /**< 기압 고도 (m, MS5611 기압→고도 변환) */
    float kf_alt_m;                  /**< 칼만 필터 융합 고도 (m, 기압 고도 입력) */
    float kf_roll_deg;               /**< Roll 각 (도, IMU SFLP 쿼터니언→Euler 변환) */
    float kf_pitch_deg;              /**< Pitch 각 (도, IMU SFLP 쿼터니언→Euler 변환) */

} telemetry_payload_sensor_snapshot_t;

/**
 * @brief 텔레메트리 프레임 (148 bytes 총)
 * @details 프레임 구조 (바이트 오프셋):
 *          [0-1]: Magic Bytes (0xA5, 0x5A) - 프레임 동기화
 *          [2]: Version (1) - 프로토콜 버전
 *          [3]: Message Type (0x01=Heartbeat, 0x02=Sensor Snapshot)
 *          [4-5]: Payload Length (바이트, Little-Endian)
 *          [6-7]: Sequence Number (프레임 순서, Little-Endian)
 *          [8-11]: Timestamp (ms, HAL_GetTick, Little-Endian)
 *          [12-143]: Payload (132 bytes, telemetry_payload_sensor_snapshot_t)
 *          [144-145]: CRC16 (CRC-16/CCITT-FALSE, Header+Payload, Little-Endian)
 * @note 50Hz 전송 (20ms 주기), UART3 (115200 baud, DMA TX)
 */
typedef struct {
    uint8_t magic[2];                /**< Magic Bytes {0xA5, 0x5A} (프레임 시작 표지) */
    uint8_t version;                 /**< 프로토콜 버전 (현재: 1) */
    uint8_t msg_type;                /**< 메시지 타입 (0x01=Heartbeat, 0x02=Sensor Snapshot) */
    uint16_t payload_len;            /**< 페이로드 길이 (바이트, sizeof(payload)) */
    uint16_t seq;                    /**< 시퀀스 번호 (0-65535, 순환) */
    uint32_t timestamp_ms;           /**< 타임스탬프 (ms, 시스템 가동 후 경과 시간) */
    telemetry_payload_sensor_snapshot_t payload; /**< 센서 스냅샷 페이로드 (132 bytes) */
    uint16_t crc16;                  /**< CRC-16/CCITT-FALSE (Header+Payload, 무결성 검증) */
} telemetry_frame_t;

#pragma pack(pop)

/**
 * @defgroup TELEMETRY_UTILS 텔레메트리 유틸리티 함수
 * @{
 */

/**
 * @brief CRC-16/CCITT-FALSE 계산
 * @param[in] data 데이터 버퍼 포인터
 * @param[in] length 데이터 길이 (바이트)
 * @return uint16_t CRC-16 체크섬
 * @details 알고리즘 파라미터:
 *          - Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
 *          - Initial Value: 0xFFFF
 *          - XOR Out: 0x0000
 *          - Reflect In/Out: false
 *          사용처: 텔레메트리 프레임 무결성 검증 (헤더+페이로드)
 * @note App_Loop()에서 Telemetry_Send() 전 자동 계산
 */
uint16_t CRC16_CCITT(uint8_t *data, uint16_t length);

/**
 * @brief 텔레메트리 프레임 전송
 * @param[in,out] frame 텔레메트리 프레임 포인터
 * @details 전송 순서:
 *          1. CRC16 계산 (헤더 + 페이로드, 12 + 132 = 144 bytes)
 *          2. frame->crc16에 저장
 *          3. UART3 DMA 전송 시작 (148 bytes)
 * @note App_Loop()에서 50Hz 주기로 호출
 *       UART3: PA2(TX), PA3(RX), 115200 baud, DMA1 Channel 2
 */
void Telemetry_Send(telemetry_frame_t *frame);

/** @} */ // end of TELEMETRY_UTILS

#ifdef __cplusplus
}
#endif

#endif /* __TELEMETRY_H */
