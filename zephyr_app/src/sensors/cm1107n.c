/**
 * @file cm1107n.c
 * @brief CM1107N CO2 센서 드라이버 - Zephyr용 직접 구현
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cm1107n, LOG_LEVEL_INF);

#define CM1107N_ADDR        0x31
#define CM1107N_REG_CO2     0x01  /* CO2 ppm */

struct cm1107n_data {
    uint16_t co2_ppm;
};

struct cm1107n_config {
    struct i2c_dt_spec i2c;
};

int cm1107n_read_co2(const struct device *dev, uint16_t *co2_ppm) {
    const struct cm1107n_config *config = dev->config;
    uint8_t reg = CM1107N_REG_CO2;
    uint8_t buf[2];
    int ret;
    
    ret = i2c_write_read_dt(&config->i2c, &reg, 1, buf, 2);
    if (ret != 0) {
        return ret;
    }
    
    *co2_ppm = (buf[0] << 8) | buf[1];
    return 0;
}

int cm1107n_init(const struct device *dev) {
    const struct cm1107n_config *config = dev->config;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    LOG_INF("CM1107N initialized");
    return 0;
}

#define CM1107N_DEFINE(inst) \
    static struct cm1107n_data cm1107n_data_##inst; \
    static const struct cm1107n_config cm1107n_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_INST(inst, cubic_cm1107n)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, cm1107n_init, NULL, \
                          &cm1107n_data_##inst, \
                          &cm1107n_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(CM1107N_DEFINE)
