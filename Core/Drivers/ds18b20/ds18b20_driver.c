#include "ds18b20_driver.h"
#include "ds18b20.h"

// The library uses global state or we need to scan ROMs.
// Simplified wrapper:
// We assume we have 2 sensors.
// 0: Battery
// 1: Board

void DS18B20_Init_Driver(void) {
    ds18b20_init(); // internal lib init
}

int16_t DS18B20_ReadTemp_x100(uint8_t sensor_idx) {
    // In real system, we address by ROM.
    // Here we just ask for device 0 or 1 found on bus.
    // ds18b20_getTemp(device_index) returning float.
    
    float t = ds18b20_getTemp(sensor_idx);
    return (int16_t)(t * 100.0f);
}
