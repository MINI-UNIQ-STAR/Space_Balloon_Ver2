/**
 * @file test_main.c
 * @brief GPS NMEA 파싱 및 체크섬 검증 테스트 (SITL)
 * @details XA1110 드라이버의 NMEA 파서 견고성 테스트
 *          - GGA, RMC, GSV 등 표준 문장 파싱
 *          - 체크섬 계산 및 유효성 검증
 *          - 잘못된 형식(Malformed) 또는 깨진 데이터 거부 로직
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "unity.h"
#include "xa1110_driver.h"
#include <string.h>

/** @brief 테스트용 드라이버 컨텍스트 */
static xa1110_ctx_t test_ctx;

void setUp(void) {
    // 테스트 컨텍스트 초기화
    memset(&test_ctx, 0, sizeof(xa1110_ctx_t));
}

void tearDown(void) {
    // 테스트 종료 후 정리 (필요 시)
}

/**
 * @brief 테스트 1: 올바른 체크섬을 가진 유효한 NMEA 문장 검증
 */
void test_valid_gga_sentence(void) {
    // 유효한 GGA 문장 (체크섬 *47 포함)
    char valid_gga[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool result = XA1110_ParseSentence(&test_ctx, valid_gga);

    // 파싱 성공 기대
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(1, test_ctx.data.fix_type);  // Fix 품질 = 1
    TEST_ASSERT_EQUAL_UINT8(8, test_ctx.data.sats_used);  // 사용 위성 수 = 8
}

/**
 * @brief 테스트 2: 잘못된 체크섬 - 거부되어야 함
 */
void test_invalid_checksum(void) {
    // 잘못된 체크섬을 가진 GGA 문장 (마지막 글자를 *48로 변경)
    char invalid_gga[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48";

    bool result = XA1110_ParseSentence(&test_ctx, invalid_gga);

    // 체크섬 불일치로 거부
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 테스트 3: 유효한 RMC 문장
 */
void test_valid_rmc_sentence(void) {
    // 유효한 RMC 문장
    char valid_rmc[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";

    bool result = XA1110_ParseSentence(&test_ctx, valid_rmc);

    // 파싱 성공 기대
    TEST_ASSERT_TRUE(result);
    // NMEA 포맷 검증: 4807.038 = 48도 + 7.038분
    // 여기서는 파싱 성공 여부와 Fix 타입 갱신 여부만 확인
    TEST_ASSERT_EQUAL_UINT8(2, test_ctx.data.fix_type);  // Valid = Fix Type 2
}

/**
 * @brief 테스트 4: 형식이 잘못된 문장 ($ 기호 누락)
 */
void test_malformed_no_dollar(void) {
    // 시작 부분에 $ 기호 누락
    char malformed[] = "GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool result = XA1110_ParseSentence(&test_ctx, malformed);

    // 잘못된 형식이므로 거부
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 테스트 5: 유효한 GSV 문장 (GPS 위성 정보)
 */
void test_valid_gsv_gps(void) {
    // GPS 위성 정보가 담긴 유효한 GSV 문장
    char valid_gsv[] = "$GPGSV,3,1,12,01,05,060,30,05,10,120,35,09,15,180,40,12,20,240,45*7C";

    bool result = XA1110_ParseSentence(&test_ctx, valid_gsv);

    // GSV 파싱 지원 여부와 관계없이 크래시가 발생하지 않아야 함
    (void)result;  // 결과값 미사용 (경고 방지)
}

/**
 * @brief 테스트 6: 유효한 GSV 문장 (GLONASS 위성)
 */
void test_valid_gsv_glonass(void) {
    // GLONASS 위성 정보 (GL Talker ID)
    char valid_gsv[] = "$GLGSV,2,1,08,65,05,060,30,66,10,120,35,67,15,180,40,68,20,240,45*60";

    bool result = XA1110_ParseSentence(&test_ctx, valid_gsv);

    // 크래시 없이 수행 확인
    (void)result;
}

/**
 * @brief 테스트 7: 문장 중간에 손상된 데이터 포함
 */
void test_corrupted_data(void) {
    // 출력 불가능한 제어 문자가 포함된 손상된 문장
    char corrupted[] = "$GPGGA,123519,4807\x01.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool result = XA1110_ParseSentence(&test_ctx, corrupted);

    // 유효하지 않은 문자로 인해 거부되어야 함
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 테스트 8: 빈 문자열
 */
void test_empty_sentence(void) {
    char empty[] = "";

    bool result = XA1110_ParseSentence(&test_ctx, empty);

    // 빈 문자열 거부
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 테스트 9: 체크섬이 없는 문장 (Strict 모드 비활성 시)
 */
void test_no_checksum_lenient(void) {
    // 체크섬(*XX)이 없는 문장
    // minmea 라이브러리 설정(strict=false)에 따라 허용될 수도 있음
    char no_checksum[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,";

    bool result = XA1110_ParseSentence(&test_ctx, no_checksum);

    // 설정에 따라 결과가 다를 수 있으므로 여기서는 크래시 발생 여부만 확인
    // 체크섬이 있는 문장에 대해서는 엄격히 검사하지만, 없는 문장은 구현체 정책을 따름
    (void)result;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_valid_gga_sentence);
    RUN_TEST(test_invalid_checksum);
    RUN_TEST(test_valid_rmc_sentence);
    RUN_TEST(test_malformed_no_dollar);
    RUN_TEST(test_valid_gsv_gps);
    RUN_TEST(test_valid_gsv_glonass);
    RUN_TEST(test_corrupted_data);
    RUN_TEST(test_empty_sentence);
    RUN_TEST(test_no_checksum_lenient);

    return UNITY_END();
}
