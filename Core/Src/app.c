/**
 * @file app.c
 * @brief 메인 애플리케이션 구현 - 성층권 풍선 센서 플랫폼
 * @details 50Hz 제어 루프, PID 히터 제어, 칼만 필터, FDIR 시스템 통합
 *          실행 순서:
 *          1. 센서 데이터 수집 (11개 센서)
 *          2. FDIR 온도 기반 보호 및 상태 확인
 *          3. SHT31 히터 제어 (결로 방지)
 *          4. GPS 고도 추적 및 연속성 확인
 *          5. IMU SFLP 자세 추정 (쿼터니언 → Euler)
 *          6. 칼만 필터 고도 융합
 *          7. PID 히터 제어 (배터리, 보드)
 *          8. 저전압 보호 (< 2.7V 히터 차단)
 *          9. 텔레메트리 전송 (UART3, 148바이트)
 *          10. XCP 측정값 업데이트
 *          11. FDIR 타임아웃 체크 및 복구
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "app.h"
#include "fdir.h"
#include "main.h"  // For HAL_GPIO and pin definitions
#include "sensors.h" // For SensorID definitions
#include "bsp.h" // BSP Layer
#include "pps_capture.h" // GPS 1PPS synchronization
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

/* ========================================================================== */
/* 전역 변수 정의                                                              */
/* ========================================================================== */

/**
 * @brief 배터리 히터 PID 제어기 핸들
 * @details 목표 온도: 10°C
 *          파라미터: Kp=400, Ki=6, Kd=0, Max=60%
 *          전력 예산 보호: 최대 60% 듀티 사이클
 */
PID_HandleTypeDef hpid_bat;

/**
 * @brief 보드 히터 PID 제어기 핸들
 * @details 목표 온도: 5°C
 *          파라미터: Kp=500, Ki=5, Kd=0, Max=100%
 */
PID_HandleTypeDef hpid_brd;

/**
 * @brief 칼만 필터 핸들 (고도 융합)
 * @details 입력: MS5611 기압 고도
 *          출력: 필터링된 고도 및 수직 속도 추정
 *          파라미터: dt=0.02s, Q=0.5, R=0.3
 */
KF_Handle_t hkf;

/**
 * @brief 텔레메트리 프레임 버퍼 (148바이트)
 * @details 50Hz 전송, CRC16 무결성 검증
 */
telemetry_frame_t telem_frame;

/**
 * @brief 배터리 히터 제어 출력 (0.0 ~ 60.0%)
 * @details PID 제어기 출력값, 전력 예산 보호로 60% 제한
 */
float heater_battery_cmd = 0.0f;

/**
 * @brief 보드 히터 제어 출력 (0.0 ~ 100.0%)
 * @details PID 제어기 출력값
 */
float heater_board_cmd = 0.0f;

/**
 * @brief 저전압 모드 상태 플래그
 * @details 0 = 정상 모드, 1 = 저전압 모드 (히터 차단)
 *          진입: < 2.7V, 해제: > 2.9V (200mV 히스테리시스)
 */
uint8_t g_low_voltage_mode = 0;

/* ========================================================================== */
/* 함수 구현                                                                  */
/* ========================================================================== */

/**
 * @brief 저전압 모드 상태 조회
 * @return uint8_t 0 = 정상, 1 = 저전압 모드
 */
uint8_t App_IsLowVoltageMode(void) {
    return g_low_voltage_mode;
}

/**
 * @brief 애플리케이션 초기화
 * @details 초기화 순서:
 *          0. BSP_Init() - 보드 지원 패키지 (GPIO, ADC, I2C, UART)
 *          1. Sensors_Init() - 11개 센서 초기화
 *          2. PID 제어기 초기화 (배터리, 보드 히터)
 *          3. 칼만 필터 초기화
 *          4. XCP 캘리브레이션 프로토콜 초기화
 *          5. 텔레메트리 프레임 헤더 초기화
 *          6. 액추에이터 초기화 (PWM 히터)
 *          7. FDIR 시스템 초기화
 *          8. GPS 1PPS 동기화 초기화
 * @note main() 함수에서 1회 호출
 */
void App_Init(void) {
    // 0. Board Init
    BSP_Init();

    // 1. Sensor Init
    Sensors_Init();

    // 2. Thermal PID Init
    // ** Power Budget Tuning (2026-01-09) **
    // Battery Heater: Kapton 7.2W @ 5V, limited to 60% duty (HEATER_BATT_MAX_DUTY)
    // Kp scaled down from 1000 → 400 to account for duty cycle limit
    // MaxOutput set to 60.0 to match power budget (redundant with app.c limiter, but safer)
    PID_Init(&hpid_bat, 400.0f, 6.0f, 0.0f, 60.0f); // Kp, Ki, Kd, Max
    hpid_bat.Target = 10.0f; // Maintain 10C

    // Board Heater: Minibulb ~4W @ 5V, no limit yet (pending hardware test)
    PID_Init(&hpid_brd, 500.0f, 5.0f, 0.0f, 100.0f);
    hpid_brd.Target = 5.0f; // Maintain 5C

    // 3. Kalman Init (50Hz = 0.02s)
    // Process Noise 0.5 (Balloon Dynamics), Meas Noise 0.3 (Baro Precision ~0.5m)
    KF_Init(&hkf, 0.02f, 0.5f, 0.3f);

    // 4. XCP Init
    XCP_Init();

    // 5. Telemetry Header Init
    telem_frame.magic[0] = 0xA5;
    telem_frame.magic[1] = 0x5A;
    telem_frame.version = 1;
    telem_frame.msg_type = 0x02; // Sensor Snapshot
    telem_frame.payload_len = sizeof(telemetry_payload_sensor_snapshot_t);

    // 6. Actuators Init
    Actuators_Init();

    // 7. FDIR Init
    FDIR_Init();

    // 8. GPS 1PPS Init
    PPS_Init();
}

/**
 * @brief 메인 루프 (50Hz, 20ms 주기)
 * @details 실행 순서:
 *          1. 센서 데이터 수집 (IMU, MAG, BARO, SHT, AirQuality, Battery, Temps, Rad, GPS)
 *          2. FDIR 온도 업데이트 및 센서 상태 확인 (저온 비활성화)
 *          3. SHT31 히터 제어 (0°C ~ 2°C 히스테리시스, 결로 방지)
 *          4. GPS 고도 추적 및 범위 검증
 *          5. IMU SFLP 쿼터니언 읽기 → Euler 각(Roll, Pitch) 변환
 *          6. 기압 고도 계산 (101325Pa 기준, 12Pa/m)
 *          7. 칼만 필터 예측/업데이트 (기압 고도 융합)
 *          8. 저전압 보호 체크 (< 2.7V: 히터 차단, > 2.9V: 해제)
 *          9. PID 제어로 히터 듀티 사이클 계산
 *          10. 전력 예산 보호 (배터리 히터 60% 제한)
 *          11. 액추에이터 출력 (PWM 히터)
 *          12. FDIR 상태 플래그 생성 (텔레메트리용)
 *          13. 텔레메트리 프레임 전송 (UART3, CRC16)
 *          14. XCP 측정값 업데이트 (DAQ)
 *          15. FDIR 타임아웃 체크 및 복구 시도
 * @note while(1) 루프에서 반복 호출, 실제 주기는 HAL_Delay(20) 또는 타이머로 제어
 */
void App_Loop(void) {
    /* ====================================================================== */
    /* 1. 센서 데이터 수집                                                     */
    /* ====================================================================== */

    // 1.1 기본 센서 데이터 (IMU, MAG, BARO, SHT)
    Sensors_Read_All(&telem_frame.payload);

    // 1.2 공기질 센서 (CO2, Ozone, PM1.0, PM2.5)
    Sensors_Read_AirQuality(&telem_frame.payload.co2_ppm, &telem_frame.payload.ozone_ppb, &telem_frame.payload.pm1_ugm3, &telem_frame.payload.pm25_ugm3);

    // 1.3 배터리 전압 및 온도
    Sensors_Read_Battery(&telem_frame.payload.bat_mv, &telem_frame.payload.bat_temp_c_x100);

    // 1.4 기타 온도 및 방사선
    Sensors_Read_BoardTemp(&telem_frame.payload.board_temp_c_x100);
    Sensors_Read_External(&telem_frame.payload.external_temp_c_x100);
    Sensors_Read_Rad(&telem_frame.payload.gdk101_usvh_x100);

    /* ====================================================================== */
    /* 2. FDIR 온도 기반 보호 및 센서 상태 확인                                */
    /* ====================================================================== */

    // ** Temperature-based FDIR Update **
    // Update FDIR with current external temperature for thermal protection logic
    int16_t ext_temp = telem_frame.payload.external_temp_c_x100;
    FDIR_UpdateTemperature(ext_temp);

    // Check FDIR status for Air Quality Sensors and mask data if disabled/failed
    // PMS3003: < -10°C 비활성화
    if (!FDIR_IsSensorHealthy(SENSOR_ID_PMS) || FDIR_IsSensorColdDisabled(SENSOR_ID_PMS)) {
        telem_frame.payload.pm1_ugm3 = 0xFFFF;
        telem_frame.payload.pm25_ugm3 = 0xFFFF;
        telem_frame.payload.pm10_ugm3 = 0xFFFF;
    }

    // CM1107N: < -5°C 비활성화
    if (!FDIR_IsSensorHealthy(SENSOR_ID_CO2) || FDIR_IsSensorColdDisabled(SENSOR_ID_CO2)) {
        telem_frame.payload.co2_ppm = 0xFFFF;
    }

    // GDK101: < -20°C 비활성화
    if (!FDIR_IsSensorHealthy(SENSOR_ID_RAD) || FDIR_IsSensorColdDisabled(SENSOR_ID_RAD)) {
        telem_frame.payload.gdk101_usvh_x100 = 0xFFFF;
    }

    // Ozone sensor special handling (ppb)
    if (!FDIR_IsSensorHealthy(SENSOR_ID_SHT) && !FDIR_IsSensorHealthy(SENSOR_ID_EXT_TEMP)) {
       // If both temp sensors fail, we might want to flag something, but currently just proceed
    }

    /* ====================================================================== */
    /* 3. SHT31 히터 제어 (결로 방지)                                          */
    /* ====================================================================== */

    // ** SHT31 Heater Control (Anti-condensation) **
    // Turn ON if temp < 0C, Turn OFF if temp > 2C (Hysteresis)
    float sht31_temp_c = telem_frame.payload.sht31_temp_c_x100 / 100.0f;
    static uint8_t sht31_heater_on = 0;

    if (sht31_temp_c < 0.0f) {
        if (sht31_heater_on == 0) {
            Sensors_SetHeater_SHT31(1);
            sht31_heater_on = 1;
        }
    } else if (sht31_temp_c > 2.0f) {
        if (sht31_heater_on == 1) {
            Sensors_SetHeater_SHT31(0);
            sht31_heater_on = 0;
        }
    }

    /* ====================================================================== */
    /* 4. GPS 데이터 수집 및 고도 추적                                          */
    /* ====================================================================== */

    // 7. GPS
    Sensors_Read_GPS(&telem_frame.payload.gps_lat_deg_e7, &telem_frame.payload.gps_lon_deg_e7,
                     &telem_frame.payload.gps_alt_m, &telem_frame.payload.gps_fix,
                     &telem_frame.payload.gps_sats_used, &telem_frame.payload.gps_sats_in_view_total,
                     &telem_frame.payload.gps_sats_in_view_gps, &telem_frame.payload.gps_sats_in_view_glonass,
                     &telem_frame.payload.gps_sats_in_view_galileo, &telem_frame.payload.gps_sats_in_view_beidou,
                     &telem_frame.payload.gps_utc_hour, &telem_frame.payload.gps_utc_min, &telem_frame.payload.gps_utc_sec,
                     &telem_frame.payload.gps_utc_day, &telem_frame.payload.gps_utc_month, &telem_frame.payload.gps_utc_year);

    // ** FDIR GPS Altitude Tracking (Range + Continuity) **
    FDIR_UpdateGPSAltitude(telem_frame.payload.gps_alt_m);

    /* ====================================================================== */
    /* 5. 자세 추정 (IMU SFLP 쿼터니언 → Euler 각 변환)                         */
    /* ====================================================================== */

    // ** Attitude Estimation (SFLP) **
    float quat[4]; // x, y, z, w
    float roll_deg = 0.0f;
    float pitch_deg = 0.0f;

    Sensors_Read_SFLP(quat);

    // Quaternion to Euler (Roll, Pitch) conversion
    // Assuming quat order: [x, y, z, w]
    float qx = quat[0];
    float qy = quat[1];
    float qz = quat[2];
    float qw = quat[3];

    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    roll_deg = atan2f(sinr_cosp, cosr_cosp) * (180.0f / 3.14159265f);

    // Pitch (y-axis rotation)
    float sinp = 2.0f * (qw * qy - qz * qx);
    if (fabsf(sinp) >= 1)
        pitch_deg = copysignf(90.0f, sinp); // use 90 degrees if out of range
    else
        pitch_deg = asinf(sinp) * (180.0f / 3.14159265f);

    telem_frame.payload.kf_roll_deg = roll_deg;
    telem_frame.payload.kf_pitch_deg = pitch_deg;

    /* ====================================================================== */
    /* 6. 기압 고도 계산 및 범위 검증                                           */
    /* ====================================================================== */

    // Convert fixed point to float for Algorithms
    float current_battery_temp = telem_frame.payload.bat_temp_c_x100 / 100.0f;
    float current_board_temp = telem_frame.payload.board_temp_c_x100 / 100.0f;

    /* FDIR Baro Range Validation */
    if (!FDIR_ValidateRange_Baro(telem_frame.payload.ms5611_press_pa)) {
        telem_frame.payload.ms5611_press_pa = 101325U; /* Use sea level as fallback */
    }

    // Barometric Altitude (Approx)
    // P0=101325, Lapse Rate can be added later. Linear approx near sea level: 12Pa per meter.
    if (telem_frame.payload.ms5611_press_pa == 0) telem_frame.payload.ms5611_press_pa = 101325; // Prevent jump if 0
    float baro_alt = (101325.0f - (float)telem_frame.payload.ms5611_press_pa) / 12.0f;

    // ** FDIR Baro Altitude Tracking **
    FDIR_UpdateBaroAltitude(baro_alt);

    telem_frame.payload.press_alt_m = baro_alt;

    /* ====================================================================== */
    /* 7. 칼만 필터 고도 융합                                                  */
    /* ====================================================================== */

    /* Kalman Initial Convergence */
    static uint8_t kf_initialized = 0U;
    if ((kf_initialized == 0U) && (baro_alt > -1000.0f) && (baro_alt < 40000.0f)) {
        hkf.x[0] = baro_alt; /* Initialize State to Measurement */
        kf_initialized = 1U;
    }

    /* 3. Kalman Update (Predict --> Update) */
    KF_Predict(&hkf);
    KF_Update_Altitude(&hkf, baro_alt);

    KF_CheckDivergence(&hkf);  /* FMEA W-05: Check and reset if diverged */

    telem_frame.payload.kf_alt_m = hkf.x[0];

    /* ====================================================================== */
    /* 8. 저전압 보호 (Load Shedding)                                         */
    /* ====================================================================== */

    // ** Low Voltage Protection (Load Shedding) **
    // Disable high-power consumers when battery voltage < 2.7V to prevent brownout
    uint16_t bat_mv = telem_frame.payload.bat_mv;

    if (bat_mv < 2700 && g_low_voltage_mode == 0) {
        // Enter low voltage mode
        g_low_voltage_mode = 1;

        // Disable heaters (highest power consumers)
        heater_battery_cmd = 0.0f;
        heater_board_cmd = 0.0f;

        // Disable PMS3003 (UART2 sensor, moderate power ~100mA)
        // Note: PMS3003 will be re-enabled when voltage recovers
        #ifndef HOST_TEST_MODE
        // No explicit disable function for PMS yet, but we can stop reading it
        // FDIR will mark it as timeout if we don't update it
        #endif
    }
    else if (bat_mv > 2900 && g_low_voltage_mode == 1) {
        // Exit low voltage mode with 200mV hysteresis (2.9V)
        g_low_voltage_mode = 0;
    }

    /* ====================================================================== */
    /* 9. PID 히터 제어 및 전력 예산 보호                                       */
    /* ====================================================================== */

    // 2. PID Update (skipped if in low voltage mode)
    if (g_low_voltage_mode == 0) {
        heater_battery_cmd = PID_Update(&hpid_bat, current_battery_temp, 0.02f);
        heater_board_cmd = PID_Update(&hpid_brd, current_board_temp, 0.02f);

        // ** Power Budget Protection: Limit heater duty cycles **
        // Kapton heater: 7.2W @ 5V → 1.44A max current
        // Limit to 60% duty to ensure 3+ hour flight time with 2500mAh battery
        if (heater_battery_cmd > HEATER_BATT_MAX_DUTY) {
            heater_battery_cmd = HEATER_BATT_MAX_DUTY;
        }

        // Minibulb: No limit for now (TBD based on hardware test)
        if (heater_board_cmd > HEATER_BOARD_MAX_DUTY) {
            heater_board_cmd = HEATER_BOARD_MAX_DUTY;
        }
    }
    else {
        // Keep heaters off in low voltage mode
        heater_battery_cmd = 0.0f;
        heater_board_cmd = 0.0f;
    }

    /* ====================================================================== */
    /* 10. 액추에이터 출력                                                     */
    /* ====================================================================== */

    // 3. Actuator Output
    Actuators_SetHeater_Battery(heater_battery_cmd);
    Actuators_SetHeater_Board(heater_board_cmd);

    // Update Telemetry with Control Output
    telem_frame.payload.heater_bat_duty_percent = (uint8_t)heater_battery_cmd;
    telem_frame.payload.heater_board_duty_percent = (uint8_t)heater_board_cmd;

    /* ====================================================================== */
    /* 11. FDIR 상태 플래그 생성                                               */
    /* ====================================================================== */

    // ** FDIR Status Flags Update **
    telem_frame.payload.status_flags = FDIR_GetStatusFlags();

    /* Add heater active flag */
    if ((heater_battery_cmd > 1.0f) || (heater_board_cmd > 1.0f)) {
        telem_frame.payload.status_flags |= STATUS_HEATER_ACTIVE;
    }

    /* ====================================================================== */
    /* 12. 텔레메트리 전송                                                     */
    /* ====================================================================== */

    // 4. Update Header
    telem_frame.seq++;
    telem_frame.timestamp_ms += 20; // Simulated time

    // 5. XCP DAQ
    XCP_UpdateMeasurements();

    // 6. Telemetry Transmit
    Telemetry_Send(&telem_frame);

    /* ====================================================================== */
    /* 13. FDIR 타임아웃 체크 및 복구                                           */
    /* ====================================================================== */

    // 7. FDIR Update
    FDIR_Update();
}
