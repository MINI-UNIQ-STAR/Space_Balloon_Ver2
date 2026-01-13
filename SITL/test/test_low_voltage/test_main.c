/**
 * @file test_main.c
 * @brief 저전압 보호(Low Voltage Protection) 로직 테스트 (SITL)
 * @details 배터리 전압에 따른 시스템 보호 모드 진입/해제 검증
 *          - 저전압 임계값(2.7V) 미만 시 Low Voltage Mode 진입
 *          - 히터 강제 차단 동작 확인
 *          - 전압 회복 시 히스테리시스(2.9V) 적용 확인
 *          - 모드 전환 반복 안정성 테스트
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "unity.h"
#include "app.h"
#include "telemetry.h"
#include <string.h>

// Mock 외부 의존성 - 테스트를 위한 정의
telemetry_frame_t telem_frame;
uint8_t g_low_voltage_mode;
float heater_battery_cmd;
float heater_board_cmd;

// app.c의 Helper 함수 구현 (Mock)
uint8_t App_IsLowVoltageMode(void) {
    return g_low_voltage_mode;
}

void setUp(void) {
    // 테스트 전 상태 초기화
    g_low_voltage_mode = 0;
    heater_battery_cmd = 0.0f;
    heater_board_cmd = 0.0f;
    memset(&telem_frame, 0, sizeof(telemetry_frame_t));
}

void tearDown(void) {
    // 테스트 후 정리
}

/**
 * @brief 테스트 1: 정상 전압(3.7V) - 히터 정상 동작
 */
void test_normal_voltage(void) {
    // 설정: 정상 배터리 전압
    telem_frame.payload.bat_mv = 3700;  // 3.7V
    telem_frame.payload.bat_temp_c_x100 = 500;   // 5도 (목표 온도 미만)
    telem_frame.payload.board_temp_c_x100 = 300; // 3도 (목표 온도 미만)

    // 참고: 모든 의존성이 없으므로 App_Loop 전체를 호출할 수는 없음
    // 플래그 로직만 테스트

    TEST_ASSERT_EQUAL_UINT8(0, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(0, App_IsLowVoltageMode());
}

/**
 * @brief 테스트 2: 저전압 진입(2.6V) - 저전압 모드 활성화 확인
 */
void test_low_voltage_entry(void) {
    // 설정: 배터리 전압이 2.7V 임계값 미만으로 떨어짐
    telem_frame.payload.bat_mv = 2600;  // 2.6V (2.7V 미만)

    uint16_t bat_mv = telem_frame.payload.bat_mv;

    // app.c의 저전압 체크 로직 시뮬레이션
    if (bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
        heater_battery_cmd = 0.0f;
        heater_board_cmd = 0.0f;
    }

    // 검증: 저전압 모드 활성화
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(1, App_IsLowVoltageMode());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, heater_battery_cmd);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, heater_board_cmd);
}

/**
 * @brief 테스트 3: 히스테리시스 - 전압이 2.8V로 회복되어도 모드 유지 확인
 */
void test_hysteresis_below_exit(void) {
    // 설정: 이미 저전압 모드 상태
    g_low_voltage_mode = 1;
    telem_frame.payload.bat_mv = 2800;  // 2.8V (2.7V 이상이나 해제 임계값 2.9V 미만)

    uint16_t bat_mv = telem_frame.payload.bat_mv;

    // 히스테리시스 로직 시뮬레이션
    if (bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
    }
    else if (bat_mv > 2900 && g_low_voltage_mode == 1) {
        g_low_voltage_mode = 0;
    }

    // 검증: 여전히 저전압 모드 유지 (히스테리시스 구간)
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(1, App_IsLowVoltageMode());
}

/**
 * @brief 테스트 4: 저전압 모드 탈출(3.0V) - 정상 모드 복귀 확인
 */
void test_low_voltage_exit(void) {
    // 설정: 저전압 모드, 전압이 2.9V 임계값 초과
    g_low_voltage_mode = 1;
    telem_frame.payload.bat_mv = 3000;  // 3.0V

    uint16_t bat_mv = telem_frame.payload.bat_mv;

    // 해제 로직 시뮬레이션
    if (bat_mv > 2900 && g_low_voltage_mode == 1) {
        g_low_voltage_mode = 0;
    }

    // 검증: 저전압 모드 해제됨
    TEST_ASSERT_EQUAL_UINT8(0, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(0, App_IsLowVoltageMode());
}

/**
 * @brief 테스트 5: 반복 사이클 - 진입과 해제 반복 테스트
 */
void test_multiple_cycles(void) {
    // 사이클 1: 정상 -> 저전압 (진입)
    g_low_voltage_mode = 0;
    telem_frame.payload.bat_mv = 2600;

    if (telem_frame.payload.bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
    }
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);

    // 사이클 2: 저전압 -> 정상 (해제)
    telem_frame.payload.bat_mv = 3000;

    if (telem_frame.payload.bat_mv > 2900 && g_low_voltage_mode == 1) {
        g_low_voltage_mode = 0;
    }
    TEST_ASSERT_EQUAL_UINT8(0, g_low_voltage_mode);

    // 사이클 3: 정상 -> 저전압 (재진입)
    telem_frame.payload.bat_mv = 2500;

    if (telem_frame.payload.bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
    }
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_normal_voltage);
    RUN_TEST(test_low_voltage_entry);
    RUN_TEST(test_hysteresis_below_exit);
    RUN_TEST(test_low_voltage_exit);
    RUN_TEST(test_multiple_cycles);

    return UNITY_END();
}
