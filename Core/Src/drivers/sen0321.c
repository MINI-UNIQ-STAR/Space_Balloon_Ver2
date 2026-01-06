#include "drivers/sen0321.h"

#include "drivers/i2c_bus_lock.h"

#define SEN0321_DEFAULT_ADDR (0x70 << 1) // 7-bit address 0x70 shifted left

#define REG_MODE          0x03
#define REG_SET_PASSIVE   0x04
#define REG_AUTO_DATA_H   0x09
#define REG_AUTO_DATA_L   0x0A

#define MODE_AUTOMATIC    0x00
#define MODE_PASSIVE      0x01

bool sen0321_init(sen0321_t *dev, I2C_HandleTypeDef *hi2c) {
    dev->hi2c = hi2c;
    dev->addr = SEN0321_DEFAULT_ADDR;

    // Set to Automatic Mode
    uint8_t data = MODE_AUTOMATIC;
	if (!i2c_bus_take(dev->hi2c, 0u)) {
		return false;
	}
    if (HAL_I2C_Mem_Write(dev->hi2c, dev->addr, REG_MODE, I2C_MEMADD_SIZE_8BIT, &data, 1, 100) != HAL_OK) {
		i2c_bus_give(dev->hi2c);
        return false;
    }
	i2c_bus_give(dev->hi2c);

    return true;
}

bool sen0321_read_ppb(sen0321_t *dev, int16_t *ppb) {
    uint8_t buf[2];

    // In Automatic mode, we just read the data registers.
    // The sensor updates them automatically.
	if (!i2c_bus_take(dev->hi2c, 0u)) {
		return false;
	}
    if (HAL_I2C_Mem_Read(dev->hi2c, dev->addr, REG_AUTO_DATA_H, I2C_MEMADD_SIZE_8BIT, buf, 2, 100) != HAL_OK) {
		i2c_bus_give(dev->hi2c);
        return false;
    }
	i2c_bus_give(dev->hi2c);

    *ppb = (int16_t)((buf[0] << 8) | buf[1]);
    return true;
}
