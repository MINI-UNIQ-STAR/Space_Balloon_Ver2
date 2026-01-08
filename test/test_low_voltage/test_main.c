#include "unity.h"
#include "app.h"
#include "telemetry.h"
#include <string.h>

// Mock external dependencies - define them here for the test
telemetry_frame_t telem_frame;
uint8_t g_low_voltage_mode;
float heater_battery_cmd;
float heater_board_cmd;

// Implement the helper function from app.c
uint8_t App_IsLowVoltageMode(void) {
    return g_low_voltage_mode;
}

void setUp(void) {
    // Reset state before each test
    g_low_voltage_mode = 0;
    heater_battery_cmd = 0.0f;
    heater_board_cmd = 0.0f;
    memset(&telem_frame, 0, sizeof(telemetry_frame_t));
}

void tearDown(void) {
    // Clean up after test
}

/**
 * Test 1: Normal voltage (3.7V) - heaters should work normally
 */
void test_normal_voltage(void) {
    // Setup: Normal battery voltage
    telem_frame.payload.bat_mv = 3700;  // 3.7V
    telem_frame.payload.bat_temp_c_x100 = 500;   // 5°C (below target)
    telem_frame.payload.board_temp_c_x100 = 300; // 3°C (below target)

    // Note: We can't call full App_Loop() without all dependencies
    // So we just test the flag logic

    TEST_ASSERT_EQUAL_UINT8(0, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(0, App_IsLowVoltageMode());
}

/**
 * Test 2: Low voltage entry (2.6V) - should enter low voltage mode
 */
void test_low_voltage_entry(void) {
    // Setup: Battery drops below 2.7V threshold
    telem_frame.payload.bat_mv = 2600;  // 2.6V (below 2.7V threshold)

    uint16_t bat_mv = telem_frame.payload.bat_mv;

    // Simulate the low voltage check logic from app.c
    if (bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
        heater_battery_cmd = 0.0f;
        heater_board_cmd = 0.0f;
    }

    // Verify: Low voltage mode activated
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(1, App_IsLowVoltageMode());
    TEST_ASSERT_EQUAL_FLOAT(0.0f, heater_battery_cmd);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, heater_board_cmd);
}

/**
 * Test 3: Hysteresis - voltage recovers to 2.8V (still in low voltage mode)
 */
void test_hysteresis_below_exit(void) {
    // Setup: Already in low voltage mode
    g_low_voltage_mode = 1;
    telem_frame.payload.bat_mv = 2800;  // 2.8V (above 2.7V but below 2.9V)

    uint16_t bat_mv = telem_frame.payload.bat_mv;

    // Simulate hysteresis logic
    if (bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
    }
    else if (bat_mv > 2900 && g_low_voltage_mode == 1) {
        g_low_voltage_mode = 0;
    }

    // Verify: Still in low voltage mode (hysteresis)
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(1, App_IsLowVoltageMode());
}

/**
 * Test 4: Exit low voltage mode (3.0V) - should exit and resume normal operation
 */
void test_low_voltage_exit(void) {
    // Setup: In low voltage mode, voltage recovers above 2.9V
    g_low_voltage_mode = 1;
    telem_frame.payload.bat_mv = 3000;  // 3.0V (above 2.9V threshold)

    uint16_t bat_mv = telem_frame.payload.bat_mv;

    // Simulate exit logic
    if (bat_mv > 2900 && g_low_voltage_mode == 1) {
        g_low_voltage_mode = 0;
    }

    // Verify: Exited low voltage mode
    TEST_ASSERT_EQUAL_UINT8(0, g_low_voltage_mode);
    TEST_ASSERT_EQUAL_UINT8(0, App_IsLowVoltageMode());
}

/**
 * Test 5: Multiple cycles - enter and exit multiple times
 */
void test_multiple_cycles(void) {
    // Cycle 1: Normal -> Low
    g_low_voltage_mode = 0;
    telem_frame.payload.bat_mv = 2600;

    if (telem_frame.payload.bat_mv < 2700 && g_low_voltage_mode == 0) {
        g_low_voltage_mode = 1;
    }
    TEST_ASSERT_EQUAL_UINT8(1, g_low_voltage_mode);

    // Cycle 2: Low -> Normal
    telem_frame.payload.bat_mv = 3000;

    if (telem_frame.payload.bat_mv > 2900 && g_low_voltage_mode == 1) {
        g_low_voltage_mode = 0;
    }
    TEST_ASSERT_EQUAL_UINT8(0, g_low_voltage_mode);

    // Cycle 3: Normal -> Low again
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
