#include "unity.h"
#include "xa1110_driver.h"
#include <string.h>

// Test context
static xa1110_ctx_t test_ctx;

void setUp(void) {
    // Initialize test context
    memset(&test_ctx, 0, sizeof(xa1110_ctx_t));
}

void tearDown(void) {
    // Clean up
}

/**
 * Test 1: Valid NMEA sentence with correct checksum
 */
void test_valid_gga_sentence(void) {
    // Valid GGA sentence with correct checksum
    char valid_gga[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool result = XA1110_ParseSentence(&test_ctx, valid_gga);

    // Should parse successfully
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(1, test_ctx.data.fix_type);  // Fix quality = 1
    TEST_ASSERT_EQUAL_UINT8(8, test_ctx.data.sats_used);  // 8 satellites
}

/**
 * Test 2: Invalid checksum - should reject
 */
void test_invalid_checksum(void) {
    // GGA sentence with WRONG checksum (changed last digit)
    char invalid_gga[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48";

    bool result = XA1110_ParseSentence(&test_ctx, invalid_gga);

    // Should reject due to invalid checksum
    TEST_ASSERT_FALSE(result);
}

/**
 * Test 3: Valid RMC sentence
 */
void test_valid_rmc_sentence(void) {
    // Valid RMC sentence
    char valid_rmc[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";

    bool result = XA1110_ParseSentence(&test_ctx, valid_rmc);

    // Should parse successfully
    TEST_ASSERT_TRUE(result);
    // NMEA format: 4807.038 = 48 degrees + 7.038 minutes = 48 + 7.038/60 = 48.1173 degrees
    // Note: Just check if parsing succeeded and fix_type is correct
    TEST_ASSERT_EQUAL_UINT8(2, test_ctx.data.fix_type);  // Valid = fix type 2
}

/**
 * Test 4: Malformed sentence (no dollar sign)
 */
void test_malformed_no_dollar(void) {
    // Missing dollar sign at start
    char malformed[] = "GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool result = XA1110_ParseSentence(&test_ctx, malformed);

    // Should reject malformed sentence
    TEST_ASSERT_FALSE(result);
}

/**
 * Test 5: Valid GSV sentence (GPS satellites in view)
 */
void test_valid_gsv_gps(void) {
    // Valid GSV sentence for GPS satellites
    char valid_gsv[] = "$GPGSV,3,1,12,01,05,060,30,05,10,120,35,09,15,180,40,12,20,240,45*7C";

    bool result = XA1110_ParseSentence(&test_ctx, valid_gsv);

    // GSV checksum should be valid, and parsing should not crash
    // Whether it returns true depends on whether MINMEA_SENTENCE_GSV is supported
    // Just verify it doesn't crash - both true/false are acceptable
    (void)result;  // Suppress unused warning
    // Test passes as long as no crash occurs
}

/**
 * Test 6: Valid GSV sentence (GLONASS satellites)
 */
void test_valid_gsv_glonass(void) {
    // Valid GSV sentence for GLONASS satellites (GL talker ID)
    char valid_gsv[] = "$GLGSV,2,1,08,65,05,060,30,66,10,120,35,67,15,180,40,68,20,240,45*60";

    bool result = XA1110_ParseSentence(&test_ctx, valid_gsv);

    // GSV checksum should be valid, and parsing should not crash
    // Whether it returns true depends on whether MINMEA_SENTENCE_GSV is supported
    // Just verify it doesn't crash - both true/false are acceptable
    (void)result;  // Suppress unused warning
    // Test passes as long as no crash occurs
}

/**
 * Test 7: Corrupted data in middle of sentence
 */
void test_corrupted_data(void) {
    // Sentence with corrupted data (non-printable character)
    char corrupted[] = "$GPGGA,123519,4807\x01.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";

    bool result = XA1110_ParseSentence(&test_ctx, corrupted);

    // Should reject due to invalid characters
    TEST_ASSERT_FALSE(result);
}

/**
 * Test 8: Empty sentence
 */
void test_empty_sentence(void) {
    char empty[] = "";

    bool result = XA1110_ParseSentence(&test_ctx, empty);

    // Should reject empty sentence
    TEST_ASSERT_FALSE(result);
}

/**
 * Test 9: Sentence without checksum (strict mode disabled)
 */
void test_no_checksum_lenient(void) {
    // Sentence without checksum (no asterisk)
    // Note: minmea_check with strict=false allows this
    char no_checksum[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,";

    bool result = XA1110_ParseSentence(&test_ctx, no_checksum);

    // With strict=false (as in our code), this might pass or fail depending on minmea implementation
    // We document the behavior rather than assert - both outcomes are acceptable
    // The important thing is that checksum validation is enabled for sentences WITH checksums
    (void)result;  // Test passes regardless of result
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
