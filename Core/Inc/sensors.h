/**
 * @file sensors.h
 * @brief 센서 드라이버 인터페이스 - 11개 센서 통합 관리
 * @details I2C1 (LSM6DSV16X, MLX90393, GDK101)
 *          I2C3 (MS5611, SHT31, CM1107N, MCP9600, SEN0321)
 *          UART (XA1110 GPS, PMS3003)
 *          1-Wire (DS18B20 x2)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __SENSORS_H
#define __SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "telemetry.h"
#include "main.h" // For HAL includes if available

/**
 * @brief 센서 동작 상태 반환값
 */
typedef enum {
    SENSOR_OK = 0,       /**< 센서 정상 동작 */
    SENSOR_ERROR = 1,    /**< 센서 오류 (I2C/UART 통신 실패) */
    SENSOR_TIMEOUT = 2   /**< 센서 응답 타임아웃 */
} SensorStatus_t;

/**
 * @brief 센서 ID (FDIR 시스템 연동용)
 * @details 타임아웃 설정 (fdir.c):
 *          - IMU: 100ms (480Hz → 빠른 갱신)
 *          - MAG: 500ms (50Hz)
 *          - BARO: 1000ms (5Hz)
 *          - GPS: 5000ms (Cold Start 고려)
 *          - PMS: 5000ms (Warm-up 필요)
 *          - CO2: 5000ms (Warm-up 필요)
 *          - SHT: 3000ms (1Hz)
 *          - RAD: 3000ms (1Hz, 1분 평균)
 *          - EXT_TEMP: 3000ms (1Hz)
 */
typedef enum {
    SENSOR_ID_IMU = 0,      /**< LSM6DSV16X (6축 IMU, I2C1, 0x6B) */
    SENSOR_ID_MAG,          /**< MLX90393 (3축 자기계, I2C1, 0x0C) */
    SENSOR_ID_BARO,         /**< MS5611 (기압계, I2C3, 0x77) */
    SENSOR_ID_GPS,          /**< XA1110 (GPS, UART1, 115200 baud) */
    SENSOR_ID_PMS,          /**< PMS3003 (미세먼지, UART2, 9600 baud) */
    SENSOR_ID_CO2,          /**< CM1107N (CO2, I2C3, 0x31) */
    SENSOR_ID_SHT,          /**< SHT31-D (온습도, I2C3, 0x44) */
    SENSOR_ID_RAD,          /**< GDK101 (방사선, I2C1, 0x18) */
    SENSOR_ID_EXT_TEMP,     /**< MCP9600 (열전대, I2C3, 0x60) */
    SENSOR_ID_COUNT         /**< 센서 총 개수 (9개) */
} SensorID_t;

/**
 * @defgroup SENSORS_INIT 초기화 및 복구 함수
 * @{
 */

/**
 * @brief 모든 센서 초기화
 * @details 순서: I2C1 → I2C3 → UART → 1-Wire
 *          각 버스별 초기화 함수 호출
 * @note app.c의 App_Init()에서 1회 호출
 */
void Sensors_Init(void);

/**
 * @brief 개별 센서 리셋 (FDIR 복구용)
 * @param[in] id 리셋할 센서 ID
 * @details 3단계 복구 메커니즘:
 *          1. GPIO 하드웨어 리셋 (센서별 리셋 핀 펄스)
 *          2. I2C 버스 복구 (SCL 9-클럭 펄스, I2C1/I2C3)
 *          3. 드라이버 재초기화 (레지스터 재설정)
 * @note FDIR_Update()에서 타임아웃 발생 시 자동 호출
 */
void Sensors_Reset(SensorID_t id);

/**
 * @brief I2C1 버스 센서 초기화 (Downside Board)
 * @details 초기화 센서:
 *          - LSM6DSV16X (IMU): SFLP 활성화, 480Hz ODR
 *          - MLX90393 (자기계): 50Hz
 *          - GDK101 (방사선): 1분 평균 측정
 */
void Sensors_Init_I2C1(void);

/**
 * @brief I2C3 버스 센서 초기화 (Upside Board)
 * @details 초기화 센서:
 *          - MS5611 (기압계): PROM 보정 데이터 읽기
 *          - SHT31-D (온습도): Single-shot 모드
 *          - CM1107N (CO2): I2C 모드 설정
 *          - MCP9600 (열전대): K-Type, Cold Junction 보상
 *          - SEN0321 (오존): I2C 주소 0x70
 */
void Sensors_Init_I2C3(void);

/**
 * @brief UART 센서 초기화
 * @details 초기화 센서:
 *          - XA1110 (GPS): UART1, 115200 baud, DMA RX
 *          - PMS3003 (미세먼지): UART2, 9600 baud, DMA RX
 */
void Sensors_Init_UART(void);

/** @} */ // end of SENSORS_INIT

/**
 * @defgroup SENSORS_READ 데이터 수집 함수
 * @{
 */

/**
 * @brief 모든 센서 데이터 일괄 수집
 * @param[out] data 텔레메트리 페이로드 구조체 포인터
 * @return SensorStatus_t 전체 수집 상태 (하나라도 실패 시 ERROR)
 * @details 수집 센서:
 *          - IMU (가속도, 자이로, SFLP 쿼터니언)
 *          - 자기계 (3축 자기장)
 *          - 기압계 (압력, 온도, 고도 계산)
 *          - 온습도 (온도, 습도)
 *          주의: GPS, PMS3003, 배터리, 방사선은 별도 함수 호출 필요
 */
SensorStatus_t Sensors_Read_All(telemetry_payload_sensor_snapshot_t *data);

/**
 * @brief IMU 데이터 읽기 (가속도, 자이로)
 * @param[out] accel 가속도 [x, y, z] (m/s² × 1000)
 * @param[out] gyro 자이로 [x, y, z] (rad/s × 1000)
 * @note LSM6DSV16X, I2C1, 480Hz ODR, ±16g / ±2000dps
 */
void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]);

/**
 * @brief IMU SFLP 쿼터니언 읽기 (자세 추정용)
 * @param[out] quaternion 쿼터니언 [x, y, z, w]
 * @details SFLP (Sensor Fusion Low Power) 내장 칼만 필터 출력
 *          App_Loop()에서 Euler 각 (Roll, Pitch)으로 변환
 */
void Sensors_Read_SFLP(float quaternion[4]);

/**
 * @brief 자기계 데이터 읽기
 * @param[out] mag 자기장 [x, y, z] (µT)
 * @note MLX90393, I2C1, 50Hz
 */
void Sensors_Read_Mag(float mag[3]);

/**
 * @brief 방사선 선량율 읽기
 * @param[out] uSvh 선량율 (µSv/h × 100)
 * @note GDK101, I2C1, 1분 평균 측정
 */
void Sensors_Read_Rad(uint16_t *uSvh);

/**
 * @brief 기압계 데이터 읽기
 * @param[out] press_pa 기압 (Pa)
 * @param[out] temp_c_x100 온도 (°C × 100)
 * @note MS5611, I2C3, 24-bit ADC, PROM 보정 적용
 */
void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100);

/**
 * @brief 온습도 센서 읽기
 * @param[out] temp_c_x100 온도 (°C × 100)
 * @param[out] rh_x100 상대습도 (%RH × 100)
 * @note SHT31-D, I2C3, Single-shot High Repeatability
 */
void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100);

/**
 * @brief 공기질 센서 읽기 (CO2, 오존, 미세먼지)
 * @param[out] co2 CO2 농도 (ppm)
 * @param[out] ozone 오존 농도 (ppb)
 * @param[out] pm1_0 PM1.0 (µg/m³)
 * @param[out] pm2_5 PM2.5 (µg/m³)
 * @note CM1107N (I2C3), SEN0321 (I2C3), PMS3003 (UART2)
 *       FDIR 저온 비활성화: PMS3003 < -10°C, CM1107N < -5°C
 */
void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5);

/** @} */ // end of SENSORS_READ

/**
 * @defgroup SENSORS_ADC ADC/1-Wire/열전대 센서
 * @{
 */

/**
 * @brief 1-Wire DS18B20 초기화
 * @details 2개 센서 연결: [0]=배터리, [1]=보드
 *          변환 시간: 750ms (12-bit)
 * @note Sensors_Init()에서 자동 호출
 */
void Sensors_Init_1Wire(void);

/**
 * @brief 배터리 전압 및 온도 읽기
 * @param[out] mv 배터리 전압 (mV)
 * @param[out] temp_c_x100 배터리 온도 (°C × 100, DS18B20[0])
 * @details ADC1_IN2 (PA1), 분압비 6:1
 *          전압 계산: raw × 3300 / 4096 × 6
 */
void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100);

/**
 * @brief 보드 온도 읽기
 * @param[out] temp_c_x100 보드 온도 (°C × 100, DS18B20[1])
 */
void Sensors_Read_BoardTemp(int16_t *temp_c_x100);

/**
 * @brief 외부 온도 읽기 (열전대)
 * @param[out] temp_c_x100 외부 온도 (°C × 100, MCP9600)
 * @note K-Type 열전대, Cold Junction 자동 보상
 */
void Sensors_Read_External(int16_t *temp_c_x100);

/** @} */ // end of SENSORS_ADC

/**
 * @defgroup SENSORS_CONTROL 센서 제어 함수
 * @{
 */

/**
 * @brief SHT31 히터 제어
 * @param[in] enable 1=히터 ON, 0=히터 OFF
 * @details 결로 방지용, App_Loop()에서 히스테리시스 제어 (0°C ~ 2°C)
 */
void Sensors_SetHeater_SHT31(uint8_t enable);

/** @} */ // end of SENSORS_CONTROL

/**
 * @defgroup SENSORS_GPS GPS 데이터 수집
 * @{
 */

/**
 * @brief GPS 데이터 읽기 (위치, 시간, 위성 정보)
 * @param[out] lat 위도 (도 × 10^7)
 * @param[out] lon 경도 (도 × 10^7)
 * @param[out] alt 고도 (m)
 * @param[out] fix Fix 상태 (0=No, 1=2D, 2=3D)
 * @param[out] sats 사용 중인 위성 수
 * @param[out] sats_view 총 가시 위성 수
 * @param[out] sats_gps GPS 위성 수
 * @param[out] sats_glonass GLONASS 위성 수
 * @param[out] sats_galileo Galileo 위성 수
 * @param[out] sats_beidou BeiDou 위성 수
 * @param[out] utc_hour UTC 시 (0-23)
 * @param[out] utc_min UTC 분 (0-59)
 * @param[out] utc_sec UTC 초 (0-59)
 * @param[out] utc_day UTC 일 (1-31)
 * @param[out] utc_month UTC 월 (1-12)
 * @param[out] utc_year UTC 년 (2000-2099)
 * @details XA1110, UART1, 115200 baud
 *          NMEA 문장: GGA (위치), RMC (시간), GSV (위성)
 *          minmea 라이브러리 사용 (CRC 체크섬 검증)
 */
void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix,
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year);

/** @} */ // end of SENSORS_GPS

#ifdef __cplusplus
}
#endif

#endif /* __SENSORS_H */
