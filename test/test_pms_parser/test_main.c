#include <stdint.h>
#include <string.h>

#include <unity.h>

#include "drivers/pms_parser.h"

static void setUp_frame(uint8_t *out32)
{
	// Build a 32-byte PMS frame with length=28 and valid checksum.
	// Layout: 0..1 header, 2..3 len(0x001C), 4..29 data(26 bytes), 30..31 checksum.
	memset(out32, 0, 32);
	out32[0] = 0x42;
	out32[1] = 0x4D;
	out32[2] = 0x00;
	out32[3] = 0x1C;

	// Put atmospheric PM values at offsets 4+6..11
	// pm1=11, pm25=22, pm10=33
	out32[10] = 0x00; out32[11] = 0x0B; // 11
	out32[12] = 0x00; out32[13] = 0x16; // 22
	out32[14] = 0x00; out32[15] = 0x21; // 33

	uint32_t sum = 0;
	for (int i = 0; i < 30; i++) {
		sum += out32[i];
	}
	out32[30] = (uint8_t)((sum >> 8) & 0xFF);
	out32[31] = (uint8_t)(sum & 0xFF);
}

void setUp(void) {}
void tearDown(void) {}

void test_pms_parser_parses_frame_and_checksum(void)
{
	uint8_t frame[32];
	setUp_frame(frame);

	pms_parser_t p;
	pms_parser_init(&p);

	pms_reading_t r = {0};
	TEST_ASSERT_TRUE(pms_parser_feed(&p, frame, sizeof(frame), &r));
	TEST_ASSERT_TRUE(r.valid);
	TEST_ASSERT_EQUAL_UINT16(11u, r.pm1_ugm3);
	TEST_ASSERT_EQUAL_UINT16(22u, r.pm25_ugm3);
	TEST_ASSERT_EQUAL_UINT16(33u, r.pm10_ugm3);
}

void test_pms_parser_rejects_bad_checksum(void)
{
	uint8_t frame[32];
	setUp_frame(frame);
	frame[31] ^= 0x01; // corrupt checksum

	pms_parser_t p;
	pms_parser_init(&p);

	pms_reading_t r = {0};
	TEST_ASSERT_FALSE(pms_parser_feed(&p, frame, sizeof(frame), &r));
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_pms_parser_parses_frame_and_checksum);
	RUN_TEST(test_pms_parser_rejects_bad_checksum);
	return UNITY_END();
}
