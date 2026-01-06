#include <stdio.h>
#include "unity.h"
#include "fdir.h"
#include "mock_dependencies.h"

// Defined in fdir.c
// void FDIR_Init(void);
// void FDIR_Update(void);
// FdirState_t FDIR_GetSensorState(SensorID_t id);

void setUp(void) {
    MockSensors_ClearStats();
    FDIR_Init();
}

void tearDown(void) {
}

void test_fdir_init_healthy(void) {
    // Check all sensors init to HEALTHY
    for(int i=0; i<SENSOR_ID_COUNT; i++) {
        TEST_ASSERT_EQUAL_INT(FDIR_STATE_HEALTHY, FDIR_GetSensorState((SensorID_t)i));
        TEST_ASSERT_TRUE(FDIR_IsSensorHealthy((SensorID_t)i));
    }
}

void test_fdir_timeout_recovery(void) {
    SensorID_t target = SENSOR_ID_BARO;
    // Timeout for Baro is 1000ms
    
    // 1. Advance time 500ms -> Still Healthy
    MockHAL_AdvanceTick(500);
    FDIR_Update();
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_HEALTHY, FDIR_GetSensorState(target));
    
    // 2. Advance time 1100ms -> Timeout -> RECOVERY
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, FDIR_GetSensorState(target));
    // Verify Sensors_Reset called (might be called for others too, so check >= 1)
    TEST_ASSERT_TRUE(MockSensors_GetResetCount() >= 1);
    TEST_ASSERT_EQUAL_INT(target, MockSensors_GetLastResetSensor());
}

void test_fdir_permanent_failure(void) {
    SensorID_t target = SENSOR_ID_BARO; // Max Recovery = 2
    
    // Initial Timeout -> Recovery 1
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    // Recovery 2
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, FDIR_GetSensorState(target));
    TEST_ASSERT_TRUE(MockSensors_GetResetCount() >= 2);
    TEST_ASSERT_EQUAL_INT(target, MockSensors_GetLastResetSensor());
    
    // Assume sensor update still fails (last_valid_update not refreshed)
    // Advance another 1100ms
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    // Recovery 2 -> Max Attempts -> FAILS PERMANENTLY here
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_FAILURE_PERMANENT, FDIR_GetSensorState(target));
    
    // Advance another 1100ms -> Max attempts reached -> FAIL
    MockHAL_AdvanceTick(1100);
    FDIR_Update();
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_FAILURE_PERMANENT, FDIR_GetSensorState(target));
}

void test_fdir_cold_protection(void) {
    // PMS3003 Min Temp is -10C (-1000)
    // Current Temp 25C -> 2500
    
    // 1. Set Temp to -20C -> Should Disable PMS
    FDIR_UpdateTemperature(-2000);
    FDIR_Update();
    
    TEST_ASSERT_TRUE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
    // PMS_SET Pin should be RESET (Low)
    TEST_ASSERT_EQUAL_INT(0, MockGPIO_GetPMSSetState());
    
    // 2. Set Temp to -5C (Hysteresis 5C, so threshold is -10+5 = -5)
    // Should still be disabled at exactly boundary or need > -5
    FDIR_UpdateTemperature(-600); 
    FDIR_Update();
    TEST_ASSERT_TRUE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
    
    // 3. Set Temp to 0C -> Should Enable
    FDIR_UpdateTemperature(0);
    FDIR_Update();
    TEST_ASSERT_FALSE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
    TEST_ASSERT_EQUAL_INT(1, MockGPIO_GetPMSSetState()); // GPIO Set
    
    // Verify State is RECOVERY (to restart sensor logic)
    TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, FDIR_GetSensorState(SENSOR_ID_PMS));
}

int main(void) {
    UnityBegin();
    

    RUN_TEST(test_fdir_init_healthy);
    RUN_TEST(test_fdir_timeout_recovery);
    RUN_TEST(test_fdir_permanent_failure);
    RUN_TEST(test_fdir_cold_protection);
    
    return UnityEnd();
}
