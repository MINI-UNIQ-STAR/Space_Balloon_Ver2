#include "drivers/mlx90393.h"
#include "stm32g4xx_hal.h"
#include "drivers/i2c_bus_lock.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

static uint8_t s_addr = MLX90393_I2C_ADDR_DEFAULT;
static volatile bool s_drdy = false;

// Commands
#define CMD_SB 0x10 // Start Burst
#define CMD_RM 0x40 // Read Measurement
#define CMD_EX 0x80 // Exit
#define CMD_WR 0x60 // Write Register

// Arguments
// Bits 3:0 -> T, Z, Y, X
#define ARG_XYZ 0x0E // 0ZYX -> 1110 -> Z, Y, X

// Registers
#define REG_GAIN_SEL 0x00
#define REG_RES_XYZ  0x02

// Sensitivity (uT/LSB) for Gain=0 (1x), Res=0 (16-bit)
// X/Y: ~0.161 uT/LSB
// Z:   ~0.294 uT/LSB
static const float SCALE_XY = 0.161f;
static const float SCALE_Z  = 0.294f;

static bool mlx90393_write_reg(uint8_t reg, uint16_t val) {
    uint8_t buf[4];
    buf[0] = CMD_WR;
    buf[1] = val >> 8;   // Data High
    buf[2] = val & 0xFF; // Data Low
    buf[3] = reg << 2;   // Address shifted left by 2

    if (!i2c_bus_take(&hi2c1, 0u)) {
        return false;
    }
    const bool ok = (HAL_I2C_Master_Transmit(&hi2c1, s_addr, buf, 4, 10) == HAL_OK);
    i2c_bus_give(&hi2c1);
    return ok;
}

bool mlx90393_init(const mlx90393_config_t *cfg) {
    if (cfg) {
        s_addr = cfg->i2c_addr;
    }

    // Exit any previous mode
    uint8_t cmd = CMD_EX;
    if (!i2c_bus_take(&hi2c1, 0u)) {
        return false;
    }
    const bool ex_ok = (HAL_I2C_Master_Transmit(&hi2c1, s_addr, &cmd, 1, 10) == HAL_OK);
    i2c_bus_give(&hi2c1);
    if (!ex_ok) {
        return false;
    }
    HAL_Delay(10);

    // Configure Gain = 0 (1x) -> Register 0x00
    // GAIN_SEL [6:4] = 0
    if (!mlx90393_write_reg(REG_GAIN_SEL, 0x0000)) {
        return false;
    }

    // Configure Resolution = 0 (16-bit) -> Register 0x02
    // RES_X/Y/Z [10:5] = 0
    if (!mlx90393_write_reg(REG_RES_XYZ, 0x0000)) {
        return false;
    }

    // Start Burst Mode (XYZ)
    // Command: 0x10 | 0x0E = 0x1E
    cmd = CMD_SB | ARG_XYZ; 
    if (!i2c_bus_take(&hi2c1, 0u)) {
        return false;
    }
    const bool sb_ok = (HAL_I2C_Master_Transmit(&hi2c1, s_addr, &cmd, 1, 10) == HAL_OK);
    i2c_bus_give(&hi2c1);
    if (!sb_ok) {
        return false;
    }
    
    return true;
}

bool mlx90393_read_data(float *x, float *y, float *z) {
    // Send RM command
    uint8_t cmd = CMD_RM | ARG_XYZ;
    if (!i2c_bus_take(&hi2c1, 0u)) {
        return false;
    }
    uint8_t buf[7];
    bool ok = true;
    if (HAL_I2C_Master_Transmit(&hi2c1, s_addr, &cmd, 1, 10) != HAL_OK) {
        ok = false;
    }
    if (ok && (HAL_I2C_Master_Receive(&hi2c1, s_addr, buf, 7, 10) != HAL_OK)) {
        ok = false;
    }
    i2c_bus_give(&hi2c1);
    if (!ok) {
        return false;
    }

    // Parse data (Big Endian: MSB first)
    int16_t raw_x = (int16_t)((buf[1] << 8) | buf[2]);
    int16_t raw_y = (int16_t)((buf[3] << 8) | buf[4]);
    int16_t raw_z = (int16_t)((buf[5] << 8) | buf[6]);

    // Conversion to uT using datasheet sensitivity for Gain=0, Res=0
    if (x) *x = (float)raw_x * SCALE_XY;
    if (y) *y = (float)raw_y * SCALE_XY;
    if (z) *z = (float)raw_z * SCALE_Z;

    s_drdy = false;
    return true;
}

void mlx90393_exti_callback(uint16_t pin) {
    s_drdy = true;
}
