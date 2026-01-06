#include <stdint.h>

#include "unity.h"

#include "drivers/sht31_codec.h"

void setUp(void) {}
void tearDown(void) {}

static void build_buf(uint16_t raw_t, uint16_t raw_rh, uint8_t out[6])
{
	out[0] = (uint8_t)(raw_t >> 8);
	out[1] = (uint8_t)(raw_t & 0xFFu);
	out[2] = sht31_crc8(&out[0], 2u);
	out[3] = (uint8_t)(raw_rh >> 8);
	out[4] = (uint8_t)(raw_rh & 0xFFu);
	out[5] = sht31_crc8(&out[3], 2u);
}

void test_sht31_parse_ok_and_crc_rejects(void)
{
	uint8_t buf[6];
	build_buf(0x1234u, 0xBEEFu, buf);

	uint16_t raw_t = 0;
	uint16_t raw_rh = 0;
	TEST_ASSERT_TRUE(sht31_parse_measurement(buf, &raw_t, &raw_rh));
	TEST_ASSERT_EQUAL_UINT16(0x1234u, raw_t);
	TEST_ASSERT_EQUAL_UINT16(0xBEEFu, raw_rh);

	buf[1] ^= 0x01u; // break CRC
	TEST_ASSERT_FALSE(sht31_parse_measurement(buf, &raw_t, &raw_rh));
}

void test_sht31_temp_conversion_endpoints(void)
{
	TEST_ASSERT_EQUAL_INT16(-4500, sht31_temp_c_x100_from_raw(0u));
	TEST_ASSERT_EQUAL_INT16(13000, sht31_temp_c_x100_from_raw(65535u));

	// mid-ish point: raw=32768 => approx 42.5C
	const int16_t t = sht31_temp_c_x100_from_raw(32768u);
	TEST_ASSERT_INT16_WITHIN(2, 4250, t);
}

void test_sht31_rh_conversion_endpoints(void)
{
	TEST_ASSERT_EQUAL_UINT16(0u, sht31_rh_x100_from_raw(0u));
	TEST_ASSERT_EQUAL_UINT16(10000u, sht31_rh_x100_from_raw(65535u));

	const uint16_t rh = sht31_rh_x100_from_raw(32768u);
	TEST_ASSERT_UINT16_WITHIN(2u, 5000u, rh);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_sht31_parse_ok_and_crc_rejects);
	RUN_TEST(test_sht31_temp_conversion_endpoints);
	RUN_TEST(test_sht31_rh_conversion_endpoints);
	return UNITY_END();
}
