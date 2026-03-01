#ifndef DS18B20_DRIVER_H
#define DS18B20_DRIVER_H

#include <stdint.h>

void DS18B20_Init_Driver(void);
void DS18B20_Recovery(void);
int16_t DS18B20_ReadTemp_x100(uint8_t sensor_idx);

#endif
