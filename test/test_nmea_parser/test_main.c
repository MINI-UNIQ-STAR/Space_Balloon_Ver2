#include <stdint.h>
#include <string.h>

#include <unity.h>

#include "services/nmea_parser.h"

static void feed_str(nmea_parser_t *p, const char *s)
{
	nmea_parser_feed(p, (const uint8_t *)s, strlen(s));
}

void setUp(void) {}
void tearDown(void) {}

void test_gga_parses_lat_lon_alt_sats_used(void)
{
	nmea_parser_t p;
	nmea_parser_init(&p);

	// Example: 48°07.038' N, 11°31.000' E, alt 545.4m, sats used 08
	feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n");

	TEST_ASSERT_TRUE(p.state.has_fix);
	TEST_ASSERT_EQUAL_INT32(481173000, p.state.lat_deg_e7);
	TEST_ASSERT_EQUAL_INT32(115166667, p.state.lon_deg_e7);
	TEST_ASSERT_EQUAL_INT32(545400, p.state.alt_mm);
	TEST_ASSERT_EQUAL_UINT8(8, p.state.sats_used);
}

void test_gsv_parses_satellites_in_view_per_constellation(void)
{
	nmea_parser_t p;
	nmea_parser_init(&p);

	// No checksum provided -> accepted by parser.
	feed_str(&p, "$GPGSV,1,1,12,01,40,083,41\r\n");
	feed_str(&p, "$GLGSV,1,1,05,65,10,123,30\r\n");
	feed_str(&p, "$GAGSV,1,1,07,11,22,333,20\r\n");
	feed_str(&p, "$BDGSV,1,1,03,02,11,222,10\r\n");

	TEST_ASSERT_EQUAL_UINT8(12, p.state.sats_in_view_gps);
	TEST_ASSERT_EQUAL_UINT8(5, p.state.sats_in_view_glonass);
	TEST_ASSERT_EQUAL_UINT8(7, p.state.sats_in_view_galileo);
	TEST_ASSERT_EQUAL_UINT8(3, p.state.sats_in_view_beidou);
	TEST_ASSERT_EQUAL_UINT8(27, p.state.sats_in_view_total);
}

void test_gn_gsv_sets_total_in_view(void)
{
	nmea_parser_t p;
	nmea_parser_init(&p);

	// No checksum provided -> accepted by parser.
	feed_str(&p, "$GNGSV,1,1,22,01,40,083,41\r\n");
	TEST_ASSERT_EQUAL_UINT8(22, p.state.sats_in_view_total);
}

void test_checksum_rejects_sentence(void)
{
	nmea_parser_t p;
	nmea_parser_init(&p);

	// Same as the GGA example but with an invalid checksum.
	feed_str(&p, "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*00\r\n");

	TEST_ASSERT_FALSE(p.state.has_fix);
	TEST_ASSERT_EQUAL_INT32(0, p.state.lat_deg_e7);
	TEST_ASSERT_EQUAL_INT32(0, p.state.lon_deg_e7);
}

void test_rmc_parses_lat_lon_and_fix(void)
{
	nmea_parser_t p;
	nmea_parser_init(&p);

	// Wikipedia example with valid checksum.
	feed_str(&p, "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n");

	TEST_ASSERT_TRUE(p.state.has_fix);
	TEST_ASSERT_EQUAL_INT32(481173000, p.state.lat_deg_e7);
	TEST_ASSERT_EQUAL_INT32(115166667, p.state.lon_deg_e7);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_gga_parses_lat_lon_alt_sats_used);
	RUN_TEST(test_gsv_parses_satellites_in_view_per_constellation);
	RUN_TEST(test_gn_gsv_sets_total_in_view);
	RUN_TEST(test_checksum_rejects_sentence);
	RUN_TEST(test_rmc_parses_lat_lon_and_fix);
	return UNITY_END();
}
