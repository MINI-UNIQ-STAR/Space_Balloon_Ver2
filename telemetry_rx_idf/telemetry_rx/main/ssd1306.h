#ifndef __SSD1306_H__
#define __SSD1306_H__

#include <driver/i2c.h>
#include <stdint.h>

void ssd1306_init(i2c_port_t i2c_num, int sda_pin, int scl_pin);
void ssd1306_clear(void);
void ssd1306_print(uint8_t page, uint8_t col, const char *str);
void ssd1306_show(void);

#endif
