/**
 * @file mock_sensors.c
 * @brief 센서 데이터 시뮬레이션 및 모킹 (SITL)
 * @details 실제 하드웨어 없이 비행 데이터를 재생하거나 수동으로 센서 값을 주입하여 테스트
 *          - 비행 데이터(.h) 기반 시뮬레이션 (자동 보간)
 *          - FDIR 테스트를 위한 고장 주입 (Fault Injection) 기능
 *          - 수동 데이터 설정 모드 (단위 테스트용)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "sensors.h"
#include "flight_data.h"
#include "fdir.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/** @brief 현재 비행 데이터 프레임 인덱스 */
static uint32_t current_frame = 0;
/** @brief 프레임 내 미세 조정을 위한 루프 카운터 */
static uint32_t loop_count = 0;

// ===== 수동 Mock 데이터 지원 =====
/** @brief 수동 데이터 모드 사용 여부 */
static bool use_manual_mock_data = false;
/** @brief 수동 설정 고도 (m) */
static float manual_alt_m = 0.0f;
/** @brief 수동 설정 기온 (°C) */
static float manual_temp_c = 25.0f;
/** @brief 수동 설정 기압 (Pa) */
static uint32_t manual_press_pa = 101325;

// ===== Fault Injection for FDIR Testing =====
/**
 * @brief FDIR 테스트용 고장 유형 열거형
 */
typedef enum {
    FAULT_NONE = 0,        /**< 정상 동작 */
    FAULT_BARO_RANGE,      /**< 기압 범위 오류 주입 (비정상적으로 높거나 낮은 값) */
    FAULT_GPS_JUMP,        /**< GPS 고도 점프 주입 (갑작스러운 500m 이상 변화) */
    FAULT_BARO_TIMEOUT,    /**< 기압 센서 응답 없음 (타임아웃 유발) */
    FAULT_GPS_TIMEOUT,     /**< GPS 센서 응답 없음 (타임아웃 유발) */
} FaultType_t;

/** @brief 현재 활성화된 고장 유형 */
static FaultType_t active_fault = FAULT_NONE;
/** @brief 고장이 시작될 프레임 번호 */
static uint32_t fault_start_frame = 0;
/** @brief 고장 지속 프레임 수 (기본값: 약 1초 @ 50Hz) */
static uint32_t fault_duration_frames = 50;

/**
 * @brief 고장 주입 (Fault Injection) 활성화
 * @param fault 주입할 고장 유형
 * @param at_frame 고장이 시작될 프레임 인덱스
 * @param duration 고장 지속 프레임 수
 */
void MockSensors_InjectFault(FaultType_t fault, uint32_t at_frame, uint32_t duration) {
    active_fault = fault;
    fault_start_frame = at_frame;
    fault_duration_frames = duration;
    printf("[MockFault] Injecting fault %d at frame %u for %u frames\n", fault, at_frame, duration);
}

/**
 * @brief 고장 해제 (정상 상태 복귀)
 */
void MockSensors_ClearFault(void) {
    active_fault = FAULT_NONE;
    printf("[MockFault] Fault cleared\n");
}

/**
 * @brief 현재 프레임에서 고장이 활성화 상태인지 확인
 * @return bool true=고장 활성, false=정상
 */
static bool _is_fault_active(void) {
    return (active_fault != FAULT_NONE && 
            current_frame >= fault_start_frame && 
            current_frame < fault_start_frame + fault_duration_frames);
}

/** @brief 선형 보간 (Linear Interpolation) 함수 */
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief 센서 모듈 초기화 (Mock)
 * @details 데이터 파일 로드 및 시뮬레이션 상태 초기화
 */
void Sensors_Init(void) {
    printf("[Mock] Sensors Initialized - RS41 Flight Data Mode\n");
    printf("[Mock] Loaded %d flight data points\n", FLIGHT_DATA_COUNT);
    printf("[Mock] Altitude range: %.0fm - %.0fm\n", 
           flight_data[0].alt_m, 
           flight_data[FLIGHT_DATA_COUNT-1].alt_m);
    current_frame = 0;
    loop_count = 0;
    active_fault = FAULT_NONE;
    use_manual_mock_data = false;
}

/**
 * @brief Mock 상태 및 통계 초기화
 */
void MockSensors_ClearStats(void) {
    current_frame = 0;
    loop_count = 0;
    active_fault = FAULT_NONE;
    use_manual_mock_data = false;
}

/** @brief 센서 리셋 명령 처리 (Mock 로그 출력) */
void Sensors_Reset(SensorID_t id) {
    printf("[Mock] Sensor Reset: %d\n", id); 
}

// 초기화 함수 스텁 (Stub)
/** @brief I2C1 센서 초기화 스텁 */
void Sensors_Init_I2C1(void) {}
/** @brief I2C3 센서 초기화 스텁 */
void Sensors_Init_I2C3(void) {}
/** @brief UART 센서 초기화 스텁 */
void Sensors_Init_UART(void) {}
/** @brief 1-Wire 센서 초기화 스텁 */
void Sensors_Init_1Wire(void) {}

/**
 * @brief 수동 Mock 데이터 설정
 * @details 이 함수 호출 시 `use_manual_mock_data`가 true로 설정됨
 * @param alt_m 설정할 고도 (m)
 * @param temp_c 설정할 온도 (°C)
 * @param press_pa 설정할 기압 (Pa)
 */
void Sensors_SetMockData(float alt_m, float temp_c, float press_pa) {
    use_manual_mock_data = true;
    manual_alt_m = alt_m;
    manual_temp_c = temp_c;
    manual_press_pa = (uint32_t)press_pa;
}

/**
 * @brief 전체 센서 데이터 읽기 (Mock)
 * @param data 센서 데이터를 채울 구조체
 * @return SensorStatus_t 항상 SENSOR_OK 반환
 * @details 시뮬레이션 모드에 따라 동작:
 *          1. 수동 모드: `Sensors_SetMockData`로 설정된 값 사용
 *          2. 비행 데이터 모드: `flight_data.h`의 배열 데이터를 보간하여 사용
 *             - 10 Sub-step 보간으로 부드러운 데이터 생성
 *             - 고장 주입 활성화 시 비정상 데이터 생성
 *             - ISA 표준 대기 모델을 이용한 기압/온도 보정
 */
SensorStatus_t Sensors_Read_All(telemetry_payload_sensor_snapshot_t *data) {
    if (use_manual_mock_data) {
        // 수동 데이터 모드 (Manual Data Mode)
        data->gps_fix = 1;
        data->gps_alt_m = manual_alt_m;
        data->external_temp_c_x100 = (int16_t)(manual_temp_c * 100);
        data->board_temp_c_x100 = (int16_t)((manual_temp_c + 5.0f) * 100);
        data->ms5611_press_pa = manual_press_pa;
        data->ms5611_temp_c_x100 = data->external_temp_c_x100;
        
        // 나머지 필드는 기본값으로 채움
        data->bat_mv = 4000;
        data->gps_lat_deg_e7 = 370000000;
        data->gps_lon_deg_e7 = 1270000000;
        data->gps_sats_used = 8;
        
        FDIR_ReportSuccess(SENSOR_ID_GPS);
        FDIR_ReportSuccess(SENSOR_ID_BARO);
        return SENSOR_OK;
    }

    // 현재 프레임과 다음 프레임 데이터 포인터 (보간을 위해)
    const flight_data_point_t *cur = &flight_data[current_frame];
    const flight_data_point_t *next = &flight_data[(current_frame + 1) % FLIGHT_DATA_COUNT];
    
    // 보간 계수 (프레임당 10 단계로 부드러운 재생)
    float t = (loop_count % 10) / 10.0f;
    
    // [GPS] 데이터 보간
    data->gps_lat_deg_e7 = (int32_t)lerp((float)cur->lat_e7, (float)next->lat_e7, t);
    data->gps_lon_deg_e7 = (int32_t)lerp((float)cur->lon_e7, (float)next->lon_e7, t);
    data->gps_alt_m = lerp(cur->alt_m, next->alt_m, t);
    data->gps_fix = 1;
    data->gps_sats_used = cur->sats;
    data->gps_sats_in_view_total = cur->sats + 3;
    data->gps_sats_in_view_gps = cur->sats;
    data->gps_sats_in_view_glonass = 2;
    data->gps_sats_in_view_galileo = 1;
    data->gps_sats_in_view_beidou = 0;
    
    // [Temperature] 라디오존데 실제 데이터 또는 고도 모델 사용
    float temp_c = cur->temp_c;
    if (temp_c < -100.0f) {
        // 온도 데이터가 없으면 ISA 표준 대기 모델 적용: -6.5°C per 1000m
        temp_c = 15.0f - (data->gps_alt_m * 0.0065f);
    }
    data->external_temp_c_x100 = (int16_t)(temp_c * 100);
    data->sht31_temp_c_x100 = (int16_t)((temp_c + 30.0f) * 100); // 보드 발열 반영
    data->board_temp_c_x100 = (int16_t)((temp_c + 35.0f) * 100);
    data->bat_temp_c_x100 = (int16_t)((temp_c + 40.0f) * 100);   // 배터리 발열 반영
    
    // [Pressure] 고도 기반 기압 산출 (ISA 공식)
    float pressure_pa = 101325.0f * powf(1.0f - data->gps_alt_m / 44330.0f, 5.255f);
    data->ms5611_press_pa = (uint32_t)pressure_pa;
    data->ms5611_temp_c_x100 = data->external_temp_c_x100;
    
    // [Battery] 배터리 전압
    data->bat_mv = cur->batt_mv;
    
    // [IMU] 가속도 (상승 가속도 시뮬레이션)
    float accel_z = 9810.0f; // 1G (unit: mg)
    if (cur->vel_v > 0) {
        accel_z += (int32_t)(cur->vel_v * 100); // 상승 중 가속도 추가
    }
    data->accel_mps2_x1000[0] = 0;
    data->accel_mps2_x1000[1] = 0;
    data->accel_mps2_x1000[2] = (int32_t)accel_z;
    
    // [Gyro] 미세한 랜덤 진동 추가
    data->gyro_rads_x1000[0] = (loop_count % 3) - 1;
    data->gyro_rads_x1000[1] = (loop_count % 5) - 2;
    data->gyro_rads_x1000[2] = (loop_count % 7) - 3;
    
    // [Magnetometer] 헤딩 각도 반영
    float heading_rad = cur->heading_deg * 3.14159f / 180.0f;
    data->mag_uT[0] = 25.0f * cosf(heading_rad);
    data->mag_uT[1] = 25.0f * sinf(heading_rad);
    data->mag_uT[2] = 45.0f;
    
    // [Humidity] 고도에 따라 감소
    float rh = 50.0f - (data->gps_alt_m / 200.0f);
    if (rh < 5.0f) rh = 5.0f;
    data->sht31_rh_x100 = (uint16_t)(rh * 100);
    
    // [Air Quality] Mock 값
    data->co2_ppm = 400;
    data->pm1_ugm3 = 10;
    data->pm25_ugm3 = 15;
    data->pm10_ugm3 = 20;
    data->ozone_ppb = 30;
    
    // [Radiation] 고도에 따라 증가
    float rad = 0.1f + (data->gps_alt_m / 5000.0f) * 0.5f;
    data->gdk101_usvh_x100 = (uint16_t)(rad * 100);
    
    // 프레임 카운터 증가 및 루프 처리
    loop_count++;
    if (loop_count % 10 == 0) {
        current_frame++;
        if (current_frame >= FLIGHT_DATA_COUNT) {
            current_frame = 0;
            printf("[Mock] Flight data looped\n");
        }
    }
    
    // FDIR에 정상 상태 보고 (Mock이므로 항상 성공)
    FDIR_ReportSuccess(SENSOR_ID_GPS);
    FDIR_ReportSuccess(SENSOR_ID_BARO);
    FDIR_ReportSuccess(SENSOR_ID_IMU);
    FDIR_ReportSuccess(SENSOR_ID_MAG);
    FDIR_ReportSuccess(SENSOR_ID_SHT);
    FDIR_ReportSuccess(SENSOR_ID_EXT_TEMP);
    FDIR_ReportSuccess(SENSOR_ID_PMS);
    FDIR_ReportSuccess(SENSOR_ID_CO2);
    FDIR_ReportSuccess(SENSOR_ID_RAD);

    return SENSOR_OK;
}

// === 개별 센서 읽기 함수 (Mock) ===

/** @brief IMU 데이터 읽기 스텁 */
void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]) {
    accel[0] = 0;
    accel[1] = 0;
    accel[2] = 9810;
    gyro[0] = 0;
    gyro[1] = 0;
    gyro[2] = 0;
}

/** @brief 센서 퓨전(SFLP) 쿼터니언 읽기 스텁 */
void Sensors_Read_SFLP(float quaternion[4]) {
    // 상승 중인 풍선의 느린 회전(Tumbling) 시뮬레이션
    // Roll 축(X)을 중심으로 회전
    static float angle = 0.0f;
    angle += 0.01f; // 각도 증가
    
    // 오일러 -> 쿼터니언 변환 (Roll only)
    // qx = sin(angle/2), qw = cos(angle/2)
    
    quaternion[0] = sinf(angle * 0.5f); // x
    quaternion[1] = 0.0f;               // y
    quaternion[2] = 0.0f;               // z
    quaternion[3] = cosf(angle * 0.5f); // w (scalar)
}

/** @brief 지자기 센서 읽기 스텁 */
void Sensors_Read_Mag(float mag[3]) {
    mag[0] = 25.0f;
    mag[1] = 0.0f;
    mag[2] = 45.0f;
}

/** @brief 방사능 센서 읽기 스텁 */
void Sensors_Read_Rad(uint16_t *uSvh) {
    *uSvh = 15; // 0.15 uSv/h
}

/** @brief 기압 센서 읽기 스텁 */
void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
    *press_pa = 50000; // ~5500m 고도
    *temp_c_x100 = -2800; // -28°C
}

/** @brief 온습도 센서 읽기 스텁 */
void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100) {
    *temp_c_x100 = -2500;
    *rh_x100 = 2000;
}

/** @brief 공기질 센서 읽기 스텁 */
void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5) {
    *co2 = 400;
    *ozone = 30;
    *pm1_0 = 10;
    *pm2_5 = 15;
}

/** @brief 배터리 전압 읽기 스텁 */
void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
    *mv = flight_data[current_frame].batt_mv;
    *temp_c_x100 = 1000; // 10°C battery
}

/** @brief 보드 온도 읽기 스텁 */
void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
    *temp_c_x100 = 500; // 5°C
}

/** @brief 외부 온도 읽기 스텁 */
void Sensors_Read_External(int16_t *temp_c_x100) {
    *temp_c_x100 = (int16_t)(flight_data[current_frame].temp_c * 100);
}

/** @brief GPS 데이터 읽기 스텁 */
void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix, 
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year) {
    const flight_data_point_t *cur = &flight_data[current_frame];
    *lat = cur->lat_e7;
    *lon = cur->lon_e7;
    *alt = cur->alt_m;
    *fix = 1;
    *sats = cur->sats;
    *sats_view = cur->sats + 3;
    *sats_gps = cur->sats;
    *sats_glonass = 2;
    *sats_galileo = 1;
    *sats_beidou = 0;
    
    // Mock UTC Time
    *utc_hour = 12; *utc_min = 0; *utc_sec = 0;
    *utc_day = 1; *utc_month = 1; *utc_year = 2026;
}

/** @brief 히터 제어 스텁 */
void Sensors_SetHeater_SHT31(uint8_t enable) {
    (void)enable;
    printf("[Mock] SHT31 Heater Set: %d\n", enable);
}
