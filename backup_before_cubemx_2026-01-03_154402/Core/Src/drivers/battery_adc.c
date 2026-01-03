#include "drivers/battery_adc.h"

#include "stm32g4xx_hal.h"

#include <string.h>

// Battery measurement is specified as PA1 (ADC1_IN2) in the project spec.
// Divider ratio is board-specific; set these constants to match schematic.
// Default assumes a 2:1 divider (VBAT -> ADC pin), i.e. VBAT = Vadc * 2.
#ifndef BAT_DIVIDER_NUM
#define BAT_DIVIDER_NUM 2u
#endif
#ifndef BAT_DIVIDER_DEN
#define BAT_DIVIDER_DEN 1u
#endif

static ADC_HandleTypeDef s_hadc1;
static bool s_inited = false;

static bool adc1_init_internal(void)
{
	__HAL_RCC_ADC12_CLK_ENABLE();

	s_hadc1.Instance = ADC1;
	s_hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	s_hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	s_hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	s_hadc1.Init.GainCompensation = 0;
	s_hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	s_hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	s_hadc1.Init.LowPowerAutoWait = DISABLE;
	s_hadc1.Init.ContinuousConvMode = DISABLE;
	s_hadc1.Init.NbrOfConversion = 1;
	s_hadc1.Init.DiscontinuousConvMode = DISABLE;
	s_hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	s_hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	s_hadc1.Init.DMAContinuousRequests = DISABLE;
	s_hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	s_hadc1.Init.OversamplingMode = DISABLE;

	if (HAL_ADC_Init(&s_hadc1) != HAL_OK) {
		return false;
	}

	if (HAL_ADCEx_Calibration_Start(&s_hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
		return false;
	}

	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_2; // PA1
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;

	if (HAL_ADC_ConfigChannel(&s_hadc1, &sConfig) != HAL_OK) {
		return false;
	}

	return true;
}

bool battery_adc_init(void)
{
	if (s_inited) {
		return true;
	}
	s_inited = adc1_init_internal();
	return s_inited;
}

bool battery_adc_read(battery_reading_t *out)
{
	if (out == NULL) {
		return false;
	}
	memset(out, 0, sizeof(*out));

	if (!battery_adc_init()) {
		return false;
	}

	if (HAL_ADC_Start(&s_hadc1) != HAL_OK) {
		return false;
	}
	if (HAL_ADC_PollForConversion(&s_hadc1, 10) != HAL_OK) {
		(void)HAL_ADC_Stop(&s_hadc1);
		return false;
	}

	const uint32_t raw = HAL_ADC_GetValue(&s_hadc1) & 0x0FFFu;
	(void)HAL_ADC_Stop(&s_hadc1);

	// Approximate: Vadc_mv = raw * 3300 / 4095
	const uint32_t vadc_mv = (raw * 3300u) / 4095u;
	uint32_t vbat_mv = (vadc_mv * BAT_DIVIDER_NUM) / BAT_DIVIDER_DEN;
	if (vbat_mv > 65535u) {
		vbat_mv = 65535u;
	}

	out->valid = true;
	out->adc_raw = (uint16_t)raw;
	out->vbat_mv = (uint16_t)vbat_mv;
	return true;
}
