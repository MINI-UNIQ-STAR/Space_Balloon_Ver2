/**
 * @file mlx90393.c
 * @brief MLX90393 3축 자기계 드라이버 - Zephyr용 직접 구현
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(mlx90393, LOG_LEVEL_INF);

#define MLX90393_ADDR           0x0C
#define MLX90393_REG_SB         0x00  /* Start Burst */
#define MLX90393_REG_SW         0x01  /* Start Wake-up */
#define MLX90393_REG_SM         0x02  /* Start Single Measurement */
#define MLX90393_REG_RM         0x04  /* Read Measurement */
#define MLX90393_REG_RR         0x05  /* Read Register */
#define MLX90393_REG_WR         0x06  /* Write Register */
#define MLX90393_REG_EX         0x08  /* Exit */
#define MLX90393_REG_HR         0x0D  /* Reset */

/* Gain settings */
#define MLX90393_GAIN_5X        0x00
#define MLX90393_GAIN_4X        0x01
#define MLX90393_GAIN_3X        0x02
#define MLX90393_GAIN_2_5X      0x03
#define MLX90393_GAIN_2X        0x04
#define MLX90393_GAIN_1_67X     0x05
#define MLX90393_GAIN_1_33X     0x06
#define MLX90393_GAIN_1X        0x07

struct mlx90393_data {
    float mag_x;
    float mag_y;
    float mag_z;
    uint8_t gain;
};

struct mlx90393_config {
    struct i2c_dt_spec i2c;
};

static int mlx90393_reset(const struct device *dev) {
    const struct mlx90393_config *config = dev->config;
    uint8_t cmd = MLX90393_REG_HR;
    return i2c_write_dt(&config->i2c, &cmd, 1);
}

static int mlx90393_start_measurement(const struct device *dev) {
    const struct mlx90393_config *config = dev->config;
    uint8_t cmd[2] = { MLX90393_REG_SM, 0x00 };  /* Single measurement, all axes */
    return i2c_write_dt(&config->i2c, cmd, 2);
}

static int mlx90393_read_data(const struct device *dev, float mag[3]) {
    const struct mlx90393_config *config = dev->config;
    uint8_t cmd = MLX90393_REG_RM;
    uint8_t buf[9];
    int ret;
    
    ret = i2c_write_read_dt(&config->i2c, &cmd, 1, buf, 9);
    if (ret != 0) {
        return ret;
    }
    
    /* Parse data (status + 6 bytes + checksum) */
    int16_t x = sys_get_be16(&buf[1]);
    int16_t y = sys_get_be16(&buf[3]);
    int16_t z = sys_get_be16(&buf[5]);
    
    /* Convert to µT (simplified, depends on gain) */
    struct mlx90393_data *data = dev->data;
    float scale = 0.161f * (8.0f / (data->gain + 1));  /* µT/LSB */
    
    mag[0] = x * scale;
    mag[1] = y * scale;
    mag[2] = z * scale;
    
    return 0;
}

static int mlx90393_sample_fetch(const struct device *dev) {
    int ret;
    
    ret = mlx90393_start_measurement(dev);
    if (ret != 0) {
        return ret;
    }
    
    k_msleep(10);  /* Wait for measurement */
    
    struct mlx90393_data *data = dev->data;
    float mag[3];
    
    ret = mlx90393_read_data(dev, mag);
    if (ret != 0) {
        return ret;
    }
    
    data->mag_x = mag[0];
    data->mag_y = mag[1];
    data->mag_z = mag[2];
    
    return 0;
}

int mlx90393_init(const struct device *dev) {
    const struct mlx90393_config *config = dev->config;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    /* Reset device */
    mlx90393_reset(dev);
    k_msleep(10);
    
    struct mlx90393_data *data = dev->data;
    data->gain = MLX90393_GAIN_1X;
    
    LOG_INF("MLX90393 initialized");
    return 0;
}

#define MLX90393_DEFINE(inst) \
    static struct mlx90393_data mlx90393_data_##inst; \
    static const struct mlx90393_config mlx90393_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_INST(inst, melexis_mlx90393)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, mlx90393_init, NULL, \
                          &mlx90393_data_##inst, \
                          &mlx90393_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(MLX90393_DEFINE)
