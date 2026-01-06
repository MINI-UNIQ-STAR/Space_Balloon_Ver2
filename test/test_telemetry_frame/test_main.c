#include <stdint.h>
#include <string.h>

#include <unity.h>

#include "services/telemetry_frame.h"

static uint16_t read_u16_le(const uint8_t *p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void setUp(void) {}
void tearDown(void) {}

void test_crc16_ccitt_false_known_vector(void)
{
	// Well-known check value for CRC-16/CCITT-FALSE("123456789") == 0x29B1
	const uint8_t msg[] = {'1','2','3','4','5','6','7','8','9'};
	TEST_ASSERT_EQUAL_HEX16(0x29B1u, telemetry_crc16_ccitt_false(msg, sizeof(msg)));
}

void test_build_frame_header_and_crc(void)
{
	telemetry_payload_sensor_snapshot_t payload;
	memset(&payload, 0, sizeof(payload));
	payload.uptime_ms = 0x89ABCDEFu;
	payload.status_flags = 0x1234u;
	payload.indoor_2nd_temp_c_x100 = (int16_t)2500;
	payload.external_temp_c_x100 = (int16_t)2600;
	uint8_t out[96];

	const uint8_t msg_type = (uint8_t)TELEM_MSG_SENSOR_SNAPSHOT;
	const uint16_t seq = 0x1234u;
	const uint32_t ts = 0x89ABCDEFu;

		TEST_ASSERT_EQUAL_UINT32((uint32_t)sizeof(telemetry_payload_sensor_snapshot_t), (uint32_t)sizeof(payload));

	const size_t n = telemetry_build_frame(msg_type, (const uint8_t *)&payload, sizeof(payload), seq, ts, out, sizeof(out));
	TEST_ASSERT_EQUAL_UINT32((uint32_t)(12u + sizeof(payload) + 2u), (uint32_t)n);

	TEST_ASSERT_EQUAL_UINT8(TELEMETRY_MAGIC0, out[0]);
	TEST_ASSERT_EQUAL_UINT8(TELEMETRY_MAGIC1, out[1]);
	TEST_ASSERT_EQUAL_UINT8(TELEMETRY_VERSION, out[2]);
	TEST_ASSERT_EQUAL_UINT8(msg_type, out[3]);
	TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(payload), read_u16_le(&out[4]));
	TEST_ASSERT_EQUAL_UINT16(seq, read_u16_le(&out[6]));
	TEST_ASSERT_EQUAL_UINT32(ts, read_u32_le(&out[8]));

	TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)&payload, &out[12], sizeof(payload));

	const uint16_t crc_calc = telemetry_crc16_ccitt_false(out, 12u + sizeof(payload));
	const uint16_t crc_frame = read_u16_le(&out[12u + sizeof(payload)]);
	TEST_ASSERT_EQUAL_UINT16(crc_calc, crc_frame);
}

void test_build_frame_invalid_args(void)
{
	uint8_t out[16];
	const uint8_t payload[] = {0x01};

		TEST_ASSERT_EQUAL_UINT32(0u, telemetry_build_frame(1, payload, sizeof(payload), 0, 0, NULL, sizeof(out)));
	TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)telemetry_build_frame(1, NULL, 1, 0, 0, out, sizeof(out)));
	TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)telemetry_build_frame(1, payload, sizeof(payload), 0, 0, out, 1));
}

int main(int argc, char **argv)
{
	( void )argc;
	( void )argv;

	UNITY_BEGIN();
	RUN_TEST(test_crc16_ccitt_false_known_vector);
	RUN_TEST(test_build_frame_header_and_crc);
	RUN_TEST(test_build_frame_invalid_args);
	return UNITY_END();
}
