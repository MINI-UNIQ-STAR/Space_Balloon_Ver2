/**
 * @file sen0321.c
 * @brief SEN0321 오존 센서 드라이버 - Zephyr용 직접 구현
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sen0321, LOG_LEVEL_INF);

#define SEN0321_ADDR        0x70
#define SEN0321_REG_OZONE   0x01  /* Ozone ppb */

struct sen0321_data {
    int16_t ozone_ppb;
};

struct sen0321_config {
    struct i2c_dt_spec i2c;
};

int sen0321_read_ozone(const struct device *dev, int16_t *ozone_ppb) {
    const struct sen0321_config *config = dev->config;
    uint8_t reg = SEN0321_REG_OZONE;
    uint8_t buf[2];
    int ret;
    
    ret = i2c_write_read_dt(&config->i2c, &reg, 1, buf, 2);
    if (ret != 0) {
        return ret;
    }
    
    *ozone_ppb = (int16_t)((buf[0] << 8) | buf[1]);
    return 0;
}

int sen0321_init(const struct device *dev) {
    const struct sen0321_config *config = dev->config;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    LOG_INF("SEN0321 initialized");
    return 0;
}

#define SEN0321_DEFINE(inst) \
    static struct sen0321_data sen0321_data_##inst; \
    static const struct sen0321_config sen0321_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_INST(inst, dfrobot_sen0321)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, sen0321_init, NULL, \
                          &sen0321_data_##inst, \
                          &sen0321_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(SEN0321_DEFINE)
