#include <stdint.h>

#include "unity.h"

#include "drivers/ms5611_codec.h"

void setUp(void) {}
void tearDown(void) {}

// Datasheet example (Page 8):
// C1=40127 C2=36924 C3=23317 C4=23282 C5=33464 C6=28312
// D1=9085466 D2=8569150
// Expected: TEMP=2007 (20.07C), P=100009 (1000.09mbar == 100009Pa)
void test_ms5611_compensate_matches_datasheet_example(void)
{
	uint16_t prom[8] = {0};
	prom[1] = 40127u;
	prom[2] = 36924u;
	prom[3] = 23317u;
	prom[4] = 23282u;
	prom[5] = 33464u;
	prom[6] = 28312u;

	int32_t temp_c_x100 = 0;
	uint32_t press_pa = 0;
	TEST_ASSERT_TRUE(ms5611_compensate(prom, 9085466u, 8569150u, &temp_c_x100, &press_pa));
	TEST_ASSERT_EQUAL_INT32(2007, temp_c_x100);
	TEST_ASSERT_EQUAL_UINT32(100009u, press_pa);
}

void test_ms5611_altitude_basic_sanity(void)
{
	// Sea level should be ~0m
	const int32_t alt0 = ms5611_altitude_m_from_pressure(101325u, 101325u);
	TEST_ASSERT_INT32_WITHIN(1, 0, alt0);

	// Around 1000m in ISA corresponds to ~89875 Pa (typical). Use wide tolerance.
	const int32_t alt1 = ms5611_altitude_m_from_pressure(89875u, 101325u);
	TEST_ASSERT_INT32_WITHIN(50, 1000, alt1);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_ms5611_compensate_matches_datasheet_example);
	RUN_TEST(test_ms5611_altitude_basic_sanity);
	return UNITY_END();
}
