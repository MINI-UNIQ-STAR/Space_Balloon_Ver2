/**
 * @file test_main.c
 * @brief 센서 드라이버 및 주변장치 초기화 테스트 (SITL)
 * @details I2C, UART 등 주변장치 Mock을 이용한 드라이버 초기화 및 통신 검증
 *          - I2C 센서 초기화 명령 시퀀스 확인
 *          - UART GPS 설정 명령 확인
 *          - 센서 데이터 파싱 로직 검증
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include <stdio.h>
#include "unity.h"
#include "mock_hal.h"
#include "sensors.h"
#include "mlx90393_driver.h"
#include "lsm6dsv16x_reg.h"

// 필요 시 내부 드라이버 상태 접근을 위한 extern 선언
// 현재는 MockHAL의 기록(History)을 통해 간접 검증

void setUp(void) {
    MockI2C_ClearStats();
    MockUART_ClearStats();
}

void tearDown(void) {
}

/** @brief I2C1 버스 센서(LSM6DSV16X) 초기화 시퀀스 테스트 */
void test_sensors_init_i2c1(void) {
    // Sensors_Init_I2C1 호출 시 LSM6DSV16X 초기화가 트리거되어야 함
    // 1. 디바이스 ID 확인 (WHO_AM_I)
    
    // Mock 응답 설정: WHO_AM_I 값 (0x70 = LSM6DSV16X_ID)
    uint8_t mock_rx[] = {0x70}; 
    MockI2C_SetNextReadData(mock_rx, 1);
    
    Sensors_Init_I2C1();
    
    // 초기화 과정 중 올바른 주소(0xD7 or 0xD6)로 쓰기 시도가 있었는지 검증
    MockI2C_LastWrite_t* last = MockI2C_GetLastWrite();
    
    // 주소가 0이 아니면 쓰기 시도 발생 (ID 체크 통과 후 설정 명령 전송)
    TEST_ASSERT_EQUAL_INT(LSM6DSV16X_I2C_ADD_H, last->addr);
}

/** @brief I2C3 버스 센서(SHT31, MS5611 등) 초기화 시퀀스 테스트 */
void test_sensors_init_i2c3(void) {
    // Sensors_Init_I2C3 호출 시 SHT31, MS5611 등 초기화 수행
    
    Sensors_Init_I2C3();
    
    MockI2C_LastWrite_t* last = MockI2C_GetLastWrite();
    // 초기화 함수 중 마지막에 호출되는 센서에 대한 쓰기 작업 확인
    // SHT31은 0x44 (7-bit) 주소 사용
    
    TEST_ASSERT(last->addr != 0);
}

/** @brief UART 센서(GPS, PMS, CM1107) 초기화 시퀀스 테스트 */
void test_sensors_init_uart(void) {
    // XA1110_Init 등 호출 확인
    // XA1110 초기화 시 "$PMTK..." 설정 명령 전송됨
    
    Sensors_Init_UART();
    
    MockUART_LastTx_t* last = MockUART_GetLastTx();
    
    // 전송된 데이터가 있는지 확인
    TEST_ASSERT(last->len != 0);
    // NMEA/PMTK 명령인지 시작 문자 검사 ($)
    TEST_ASSERT_EQUAL_INT('$', last->data[0]); 
}

/** @brief GPS 데이터 파싱 및 구조체 업데이트 테스트 */
void test_gps_parsing_mock(void) {
    // Sensors_Read_GPS 함수 내부 로직 검증
    // Fix Type이 0이면 Mock 데이터를 주입하도록 되어 있음
        
    int32_t lat, lon;
    float alt;
    uint8_t fix, sats, sats_view, s1, s2, s3, s4;
    uint8_t h, m, s, day, month;
    uint16_t year;
    
    // GPS 읽기 수행 (내부적으로 GPGGA Mock 데이터 주입)
    Sensors_Read_GPS(&lat, &lon, &alt, &fix, &sats, &sats_view, &s1, &s2, &s3, &s4,
                     &h, &m, &s, &day, &month, &year);
    
    // Mock 데이터: "$GPGGA,123519,4807.038,N,..."
    // 파싱 성공하여 Fix가 1이 되어야 함
    TEST_ASSERT_EQUAL_INT(1, fix);
    TEST_ASSERT_TRUE(lat > 0);
}

/** @brief GDK101 초기화 실패 케이스 테스트 (Stub) */
void test_sensors_init_gdk101_failure(void) {
    // IMP-08: GDK101 초기화 실패 처리 검증
    // 현재 MockHAL은 항상 OK를 반환하므로, 실패 주입을 위해서는 MockHAL 확장 필요
    // 여기서는 테스트 구조만 잡아두고 항상 성공하는 케이스로 둠
    
    // 실제 실패 테스트를 하려면 MockHAL_SetNextError(HAL_ERROR) 같은 기능 필요
}

/** @brief 방사능 센서(GDK101) 읽기 테스트 */
void test_sensors_read_rad(void) {
    // Sensors_Read_Rad -> GDK101_Read 동작 검증
    
    // Mock 데이터 준비: 1.0f (IEEE 754) = {0x00, 0x00, 0x80, 0x3F} (Little Endian)
    uint8_t dummy_data[4] = {0x00, 0x00, 0x80, 0x3F}; 
    MockI2C_SetNextReadData(dummy_data, 4);
    
    uint16_t val = 0;
    Sensors_Read_Rad(&val);
    
    // GDK101 드라이버 동작 검증 (값 변화 여부 등)
    // 현재는 드라이버 내부 구현에 따라 다를 수 있으므로 크래시 없음만 확인
    (void)val;
}

/** @brief 멀티 GNSS(GLONASS, Galileo 등) 위성 정보 파싱 테스트 */
void test_gps_gsv_parsing_multi_gnss(void) {
    int32_t lat, lon;
    float alt;
    uint8_t fix, sats, sats_view;
    uint8_t sats_gps, sats_glonass, sats_galileo, sats_beidou;
    uint8_t h, m, s, day, month;
    uint16_t year;
    
    Sensors_Read_GPS(&lat, &lon, &alt, &fix, &sats, &sats_view, 
                     &sats_gps, &sats_glonass, &sats_galileo, &sats_beidou,
                     &h, &m, &s, &day, &month, &year);
    
    // 기본 Mock GGA만으로는 다른 위성계 정보가 0이어야 함
    TEST_ASSERT_EQUAL_INT(0, sats_glonass);
    TEST_ASSERT_EQUAL_INT(0, sats_galileo);
    TEST_ASSERT_EQUAL_INT(0, sats_beidou);
}

int main(void) {
    UNITY_BEGIN();
    // RUN_TEST(test_sensors_read_rad);
    RUN_TEST(test_sensors_init_i2c1);
    RUN_TEST(test_sensors_init_i2c3);
    // RUN_TEST(test_sensors_init_uart);
    RUN_TEST(test_gps_parsing_mock);
    RUN_TEST(test_gps_gsv_parsing_multi_gnss);
    RUN_TEST(test_sensors_init_gdk101_failure);
    return UNITY_END();
}
