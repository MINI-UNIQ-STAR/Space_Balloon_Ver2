#include <stdio.h>
#include "unity.h"
#include "mock_hal.h"
#include "sensors.h"
#include "mlx90393_driver.h"
#include "lsm6dsv16x_reg.h"

// Externs for accessing internal driver state if needed
// Or just rely on MockHAL history

void setUp(void) {
    MockI2C_ClearStats();
}

void tearDown(void) {
}

void test_sensors_init_i2c1(void) {
    // Calling Sensors_Init_I2C1 should trigger LSM6DSV16X initialization
    // 1. Read byte (device check) -> Mocking WHO_AM_I return
    
    // Prepare Mock Response for WHO_AM_I (0x70 approx, checked against LSM6DSV16X_ID)
    uint8_t mock_rx[] = {0x70}; // Assuming 0x70 matches LSM6DSV16X_ID definition
    MockI2C_SetNextReadData(mock_rx, 1);
    
    Sensors_Init_I2C1();
    
    // Verify Writes
    MockI2C_LastWrite_t* last = MockI2C_GetLastWrite();
    // Use the exact constant sensors.c uses
    TEST_ASSERT_EQUAL_INT(LSM6DSV16X_I2C_ADD_H, last->addr);
}

void test_gps_parsing_mock(void) {
    // Test Sensors_Read_GPS parsing logic
    // We can inject NMEA via UART RX mock or just verifying the Parse function if accessible?
    // sensors.c has Sensors_Read_GPS which calls FDIR_ReportSuccess.
    // It also checks `xa_ctx.data.fix_type`.
    // It mocks NMEA injection if fix is 0.
    
    // Let's call Sensors_Read_GPS
    int32_t lat, lon;
    float alt;
    uint8_t fix, sats, sats_view, s1, s2, s3, s4;
    
    Sensors_Read_GPS(&lat, &lon, &alt, &fix, &sats, &sats_view, &s1, &s2, &s3, &s4);
    
    // internal mock injection in sensors.c has:
    // "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47"
    // Expect Lat: 4807.038 N -> 48 deg 07.038 min -> 48 + 7.038/60 = 48.1173 deg
    // XA1110 driver likely converts to e7.
    // 48.1173 * 1e7 = 481173000
    
    // Let's just assert that fix is valid (1)
    // The loop in sensors.c runs XA1110_ProcessByte so parser should update fix
    
    // Note: XA1110_ProcessByte must be linked.
    
    TEST_ASSERT_EQUAL_INT(1, fix);
    TEST_ASSERT_TRUE(lat > 0);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_sensors_init_i2c1);
    RUN_TEST(test_gps_parsing_mock);
    return UnityEnd();
}
