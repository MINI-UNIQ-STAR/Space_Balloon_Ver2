/**
 * @file test_main.c
 * @brief 고장 검출 및 복구(FDIR) 로직 테스트 (SITL)
 * @details 시스템 안정성을 위한 FDIR 모듈의 단위 및 통합 테스트
 *          - 초기 상태 검증
 *          - 센서 타임아웃 감지 및 자동 복구(Reset)
 *          - 반복 실패 시 영구 고장(Permanent Failure) 천이
 *          - 저온 환경에서의 센서 보호/복구 로직
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include <stdio.h>
#include "unity.h"
#include "fdir.h"
#include "mock_dependencies.h"

// FDIR 내부 함수/상태 접근용 (주석 유지)
// void FDIR_Init(void);
// void FDIR_Update(void);
// FdirState_t FDIR_GetSensorState(SensorID_t id);

void setUp(void) {
    MockSensors_ClearStats();
    FDIR_Init();
}

void tearDown(void) {
}

/** @brief 초기 상태 검증: 모든 센서 HEALTHY */
void test_fdir_init_healthy(void) {
    // 모든 센서가 HEALTHY 상태로 초기화되었는지 확인
    for(int i=0; i<SENSOR_ID_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(FDIR_STATE_HEALTHY, FDIR_GetSensorState((SensorID_t)i));
        TEST_ASSERT_TRUE(FDIR_IsSensorHealthy((SensorID_t)i));
    }
}

/** @brief 타임아웃 발생 및 자동 복구(Recovery) 테스트 */
void test_fdir_timeout_recovery(void) {
    SensorID_t target = SENSOR_ID_BARO;
    // 기압 센서(Baro) 타임아웃 설정: 1000ms
    
    // 1. 500ms 경과 -> 여전히 HEALTHY 여야 함
    MockHAL_AdvanceTick(500);
    FDIR_Update();
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_HEALTHY, FDIR_GetSensorState(target));
    
    // 2. 1100ms 경과 (총 1600ms) -> 타임아웃 -> RECOVERY 모드 진입
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, FDIR_GetSensorState(target));
    // Sensors_Reset 함수 호출 여부 확인 (최소 1회 이상)
    TEST_ASSERT_TRUE(MockSensors_GetResetCount() >= 1);
    TEST_ASSERT_EQUAL_INT(target, MockSensors_GetLastResetSensor());
}

/** @brief 반복 실패 시 영구 고장(Permanent Failure) 천이 테스트 */
void test_fdir_permanent_failure(void) {
    SensorID_t target = SENSOR_ID_BARO; // 최대 복구 시도 횟수 = 2
    
    // 초기 타임아웃 -> Recovery 1
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    // Recovery 2 상태 확인
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, FDIR_GetSensorState(target));
    TEST_ASSERT_TRUE(MockSensors_GetResetCount() >= 2);
    TEST_ASSERT_EQUAL_INT(target, MockSensors_GetLastResetSensor());
    
    // 센서 업데이트 실패 지속 가정 (last_valid_update 미갱신)
    // 1100ms 추가 경과
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    // Recovery 2 -> 최대 시도 초과 -> 영구 고장(Permanent Failure) 천이
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_FAILURE_PERMANENT, FDIR_GetSensorState(target));
    
    // 추가 시간 경과 후에도 영구 고장 상태 유지 확인
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_FAILURE_PERMANENT, FDIR_GetSensorState(target));
}

/** @brief 저온 보호(Cold Protection) 로직 테스트 */
void test_fdir_cold_protection(void) {
    // PMS3003 최소 동작 온도: -10도 (-1000)
    // 현재 초기 온도: 25도 (2500)
    
    // 1. 온도를 -20도로 설정 -> PMS 센서 비활성화 확인
    FDIR_UpdateTemperature(-2000);
    FDIR_Update();
    
    TEST_ASSERT_TRUE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
    // PMS_SET 제어 핀이 Low(RESET) 상태여야 함
    TEST_ASSERT_EQUAL_INT(0, MockGPIO_GetPMSSetState());
    
    // 2. 온도를 -5도로 설정 (히스테리시스 5도 -> 복구 임계값 -5도)
    // -5도는 경계값이므로 여전히 비활성화 상태 유지
    FDIR_UpdateTemperature(-600); 
    FDIR_Update();
    TEST_ASSERT_TRUE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
    
    // 3. 온도를 0도로 설정 -> 센서 활성화(복구)
    FDIR_UpdateTemperature(0);
    FDIR_Update();
    TEST_ASSERT_FALSE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
    TEST_ASSERT_EQUAL_INT(1, MockGPIO_GetPMSSetState()); // GPIO High (Set)
    
    // 상태는 RECOVERY로 전환되어 센서 재초기화 수행
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, FDIR_GetSensorState(SENSOR_ID_PMS));
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_fdir_init_healthy);
    RUN_TEST(test_fdir_timeout_recovery);
    RUN_TEST(test_fdir_permanent_failure);
    RUN_TEST(test_fdir_cold_protection);
    
    return UNITY_END();
}
