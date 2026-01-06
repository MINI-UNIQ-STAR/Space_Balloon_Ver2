#include "drivers/ds18b20.h"

#include "drivers/dwt_delay.h"

#include "stm32g4xx_hal.h"

#include <string.h>

// DS18B20 is specified in the project spec as PB15.
// We avoid relying on CubeMX label defines (none present in main.h).
#define ONEWIRE_GPIO_PORT GPIOB
#define ONEWIRE_GPIO_PIN  GPIO_PIN_15

static void ow_set_output_od_low(void)
{
	GPIO_InitTypeDef init = {0};
	init.Pin = ONEWIRE_GPIO_PIN;
	init.Mode = GPIO_MODE_OUTPUT_OD;
	init.Pull = GPIO_NOPULL;
	init.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(ONEWIRE_GPIO_PORT, &init);
	HAL_GPIO_WritePin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN, GPIO_PIN_RESET);
}

static void ow_release_input_pullup(void)
{
	GPIO_InitTypeDef init = {0};
	init.Pin = ONEWIRE_GPIO_PIN;
	init.Mode = GPIO_MODE_INPUT;
	init.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(ONEWIRE_GPIO_PORT, &init);
}

static bool ow_read_pin(void)
{
	return HAL_GPIO_ReadPin(ONEWIRE_GPIO_PORT, ONEWIRE_GPIO_PIN) == GPIO_PIN_SET;
}

static bool ow_reset_pulse(void)
{
	ow_set_output_od_low();
	dwt_delay_us(480);
	ow_release_input_pullup();
	dwt_delay_us(70);
	const bool presence = !ow_read_pin();
	dwt_delay_us(410);
	return presence;
}

static void ow_write_bit(uint8_t bit)
{
	if (bit) {
		ow_set_output_od_low();
		dwt_delay_us(6);
		ow_release_input_pullup();
		dwt_delay_us(64);
	} else {
		ow_set_output_od_low();
		dwt_delay_us(60);
		ow_release_input_pullup();
		dwt_delay_us(10);
	}
}

static uint8_t ow_read_bit(void)
{
	uint8_t bit;
	ow_set_output_od_low();
	dwt_delay_us(6);
	ow_release_input_pullup();
	dwt_delay_us(9);
	bit = ow_read_pin() ? 1u : 0u;
	dwt_delay_us(55);
	return bit;
}

static void ow_write_byte(uint8_t v)
{
	for (uint8_t i = 0; i < 8; i++) {
		ow_write_bit((v >> i) & 1u);
	}
}

static uint8_t ow_read_byte(void)
{
	uint8_t v = 0;
	for (uint8_t i = 0; i < 8; i++) {
		v |= (uint8_t)(ow_read_bit() << i);
	}
	return v;
}

static uint8_t ds18_crc8(const uint8_t *data, size_t len)
{
	uint8_t crc = 0;
	for (size_t i = 0; i < len; i++) {
		uint8_t inbyte = data[i];
		for (uint8_t j = 0; j < 8; j++) {
			uint8_t mix = (crc ^ inbyte) & 0x01u;
			crc >>= 1;
			if (mix) {
				crc ^= 0x8Cu;
			}
			inbyte >>= 1;
		}
	}
	return crc;
}

#define MAX_DEVICES 2
static uint8_t s_roms[MAX_DEVICES][8];
static int s_device_count = 0;

// Forward declaration
bool ds18b20_scan(void);

void ds18b20_init(void)
{
	(void)dwt_delay_init();
	ow_release_input_pullup();
	ds18b20_scan();
}

// OneWire Search Algorithm (simplified)
bool ds18b20_scan(void)
{
	s_device_count = 0;
	memset(s_roms, 0, sizeof(s_roms));

	uint8_t last_discrepancy = 0;
	bool last_device_flag = false;
	
	while (!last_device_flag && s_device_count < MAX_DEVICES) {
		if (!ow_reset_pulse()) {
			return false; // No devices
		}
		ow_write_byte(0xF0); // Search ROM

		uint8_t discrepancy_marker = 0;
		uint8_t rom_byte_number = 0;
		uint8_t rom_byte_mask = 1;
		
		for (int bit_pos = 1; bit_pos <= 64; bit_pos++) {
			int id_bit = ow_read_bit();
			int cmp_id_bit = ow_read_bit();

			if (id_bit == 1 && cmp_id_bit == 1) {
				return false; // Error
			}

			int search_direction;
			if (id_bit != cmp_id_bit) {
				search_direction = id_bit;
			} else {
				if (bit_pos < last_discrepancy) {
					search_direction = ((s_roms[s_device_count][rom_byte_number] & rom_byte_mask) > 0);
				} else {
					search_direction = (bit_pos == last_discrepancy);
				}

				if (search_direction == 0) {
					discrepancy_marker = bit_pos;
				}
			}

			if (search_direction == 1) {
				s_roms[s_device_count][rom_byte_number] |= rom_byte_mask;
			} else {
				s_roms[s_device_count][rom_byte_number] &= ~rom_byte_mask;
			}

			ow_write_bit(search_direction);

			rom_byte_mask <<= 1;
			if (rom_byte_mask == 0) {
				rom_byte_number++;
				rom_byte_mask = 1;
			}
		}

		last_discrepancy = discrepancy_marker;
		if (last_discrepancy == 0) {
			last_device_flag = true;
		}
		
		// Check CRC
		if (ds18_crc8(s_roms[s_device_count], 8) == 0) {
			s_device_count++;
		}
	}
	return (s_device_count > 0);
}

int ds18b20_get_device_count(void)
{
	return s_device_count;
}

bool ds18b20_start_conversion_all(void)
{
	(void)dwt_delay_init();
	if (!ow_reset_pulse()) {
		return false;
	}

	// Skip ROM (single-drop assumed), Convert T
	ow_write_byte(0xCC);
	ow_write_byte(0x44);
	return true;
}

bool ds18b20_read_temperature(int index, ds18b20_reading_t *out)
{
	if (out == NULL || index < 0 || index >= s_device_count) {
		return false;
	}
	memset(out, 0, sizeof(*out));

	(void)dwt_delay_init();
	if (!ow_reset_pulse()) {
		return false;
	}

	// Match ROM
	ow_write_byte(0x55);
	for (int i = 0; i < 8; i++) {
		ow_write_byte(s_roms[index][i]);
	}

	// Read Scratchpad
	ow_write_byte(0xBE);

	uint8_t scratch[9] = {0};
	for (size_t i = 0; i < sizeof(scratch); i++) {
		scratch[i] = ow_read_byte();
	}

	if (ds18_crc8(scratch, 8) != scratch[8]) {
		return false;
	}

	const int16_t raw = (int16_t)((int16_t)scratch[0] | ((int16_t)scratch[1] << 8));
	// Raw is in units of 1/16 degC.
	// temp_c_x100 = raw * 100 / 16 = raw * 25 / 4
	const int32_t temp_x100 = ((int32_t)raw * 25) / 4;
	if (temp_x100 < (int32_t)INT16_MIN || temp_x100 > (int32_t)INT16_MAX) {
		return false;
	}

	out->valid = true;
	out->temp_c_x100 = (int16_t)temp_x100;
	return true;
}
