#include "services/aux_sensors_service.h"

#include "drivers/battery_adc.h"
#include "drivers/ds18b20.h"

#include <stddef.h>

static bool s_bat_temp_valid = false;
static int16_t s_bat_temp_c_x100 = 0;
static bool s_board_temp_valid = false;
static int16_t s_board_temp_c_x100 = 0;

static bool s_bat_valid = false;
static uint16_t s_bat_mv = 0;

static uint32_t s_next_temp_ms = 0;
static uint32_t s_temp_conv_start_ms = 0;
static bool s_temp_conv_pending = false;

static uint32_t s_temp_last_update_ms = 0;

static uint32_t s_next_bat_ms = 0;

static uint32_t s_bat_last_update_ms = 0;

void aux_sensors_service_init(void)
{
	ds18b20_init();
	(void)battery_adc_init();

	s_bat_temp_valid = false;
	s_bat_temp_c_x100 = 0;
	s_board_temp_valid = false;
	s_board_temp_c_x100 = 0;
	s_bat_valid = false;
	s_bat_mv = 0;

	s_next_temp_ms = 0;
	s_temp_conv_start_ms = 0;
	s_temp_conv_pending = false;
	s_temp_last_update_ms = 0;
	
	s_next_bat_ms = 0;
	s_bat_last_update_ms = 0;
}

void aux_sensors_service_tick(uint32_t now_ms)
{
	// DS18B20: start conversion every ~1s, read after ~100ms.
	if (!s_temp_conv_pending) {
		if ((int32_t)(now_ms - s_next_temp_ms) >= 0) {
			if (ds18b20_start_conversion_all()) {
				s_temp_conv_start_ms = now_ms;
				s_temp_conv_pending = true;
			}
			s_next_temp_ms = now_ms + 1000u;
		}
	} else {
		if ((uint32_t)(now_ms - s_temp_conv_start_ms) >= 750u) { // 750ms for 12-bit
			int count = ds18b20_get_device_count();
			for (int i = 0; i < count; i++) {
				ds18b20_reading_t r;
				if (ds18b20_read_temperature(i, &r) && r.valid) {
					if (r.temp_c_x100 != 8500) {
						// Arbitrary assignment: 0=Battery, 1=Board
						if (i == 0) {
							s_bat_temp_c_x100 = r.temp_c_x100;
							s_bat_temp_valid = true;
						} else if (i == 1) {
							s_board_temp_c_x100 = r.temp_c_x100;
							s_board_temp_valid = true;
						}
						s_temp_last_update_ms = now_ms;
					}
				}
			}
			s_temp_conv_pending = false;
		}
	}

	// Battery: read every ~1s.
	if ((int32_t)(now_ms - s_next_bat_ms) >= 0) {
		battery_reading_t br;
		if (battery_adc_read(&br) && br.valid) {
			s_bat_mv = br.vbat_mv;
			s_bat_valid = true;
			s_bat_last_update_ms = now_ms;
		}
		s_next_bat_ms = now_ms + 1000u;
	}
}

bool aux_sensors_get_bat_temp_c_x100(int16_t *out)
{
	if ((out == NULL) || !s_bat_temp_valid) {
		return false;
	}
	*out = s_bat_temp_c_x100;
	return true;
}

bool aux_sensors_get_board_temp_c_x100(int16_t *out)
{
	if ((out == NULL) || !s_board_temp_valid) {
		return false;
	}
	*out = s_board_temp_c_x100;
	return true;
}

bool aux_sensors_get_bat_mv(uint16_t *out)
{
	if ((out == NULL) || !s_bat_valid) {
		return false;
	}
	*out = s_bat_mv;
	return true;
}

bool aux_sensors_get_temp_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || (!s_bat_temp_valid && !s_board_temp_valid)) {
		return false;
	}
	*out_ms = s_temp_last_update_ms;
	return true;
}

bool aux_sensors_get_bat_last_update_ms(uint32_t *out_ms)
{
	if ((out_ms == NULL) || !s_bat_valid) {
		return false;
	}
	*out_ms = s_bat_last_update_ms;
	return true;
}

void aux_sensors_service_reset_temp(void)
{
	// Non-blocking reset: restart conversion scheduling.
	s_bat_temp_valid = false;
	s_board_temp_valid = false;
	s_temp_conv_pending = false;
	s_next_temp_ms = 0;
}
