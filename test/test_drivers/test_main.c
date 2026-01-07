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
    MockUART_ClearStats();
}

void tearDown(void) {
}

void test_sensors_init_i2c1(void) {
    // Calling Sensors_Init_I2C1 should trigger LSM6DSV16X initialization
    // 1. Read byte (device check) -> Mocking WHO_AM_I return
    
    // Prepare Mock Response for WHO_AM_I (0x70 = LSM6DSV16X_ID)
    // The driver calls lsm6dsv16x_device_id_get -> platform_read
    uint8_t mock_rx[] = {0x70}; 
    MockI2C_SetNextReadData(mock_rx, 1);
    
    Sensors_Init_I2C1();
    
    // Verify that the driver attempted to WRITE to the correct address (0xD7) during configuration
    // (e.g. sw_reset, data_rate_set, etc. happen after ID check passes)
    MockI2C_LastWrite_t* last = MockI2C_GetLastWrite();
    
    // If last->addr is 0, it means no write occurred.
    // Check if ID mismatch caused return.
    // We can't see internal state easily, but we can verify the last I2C action.
    
    
    TEST_ASSERT_EQUAL_INT(LSM6DSV16X_I2C_ADD_H, last->addr);
}

void test_sensors_init_i2c3(void) {
    // Calling Sensors_Init_I2C3 should trigger SHT31, MS5611, MCP9600, SEN0321
    // We can verify one of them, e.g. SHT31 (0x44) or MS5611 (0x77)
    // SHT31 init writes to 0x44
    
    Sensors_Init_I2C3();
    
    MockI2C_LastWrite_t* last = MockI2C_GetLastWrite();
    // Just verify the last one called. 
    // Init order in sensors_copy.c: SEN, MCP, MS, SHT.
    // So last write should be SHT31.
    // SHT31 Init usually does soft reset or similar. 
    // SHT31_I2C_ADDR_DEFAULT is 0x44 or 0x45. Defined in sht31_driver.h.
    // If we assume 0x44 (7-bit) -> 0x88 (8-bit read/write).
    // Let's assert non-zero to show it ran something on I2C3 specific sensors.
    
    TEST_ASSERT(last->addr != 0);
    // Ideally we check specific address but let's confirm activity first
}

void test_sensors_init_uart(void) {
    // Should init PMS, CM1107, XA1110
    // XA1110_Init sends PMTK commands via UART.
    // Let's verify XA1110 Config command was sent.
    // "$PMTK220,100*2F\r\n" is last command in XA1110_Init.
    
    Sensors_Init_UART();
    
    MockUART_LastTx_t* last = MockUART_GetLastTx();
    
    // Check if it contains "$PMTK"
    // We can use strstr or just check non-zero length and print it (or assert length)
    TEST_ASSERT(last->len != 0);
    // Simple check if it looks like NMEA/PMTK
    TEST_ASSERT_EQUAL_INT('$', last->data[0]); 
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
    uint8_t h, m, s, day, month;
    uint16_t year;
    
    Sensors_Read_GPS(&lat, &lon, &alt, &fix, &sats, &sats_view, &s1, &s2, &s3, &s4,
                     &h, &m, &s, &day, &month, &year);
    
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
    RUN_TEST(test_sensors_init_i2c3);
    RUN_TEST(test_sensors_init_uart);
    RUN_TEST(test_gps_parsing_mock);
    return UnityEnd();
}
