/**
 * @file gdk101.c
 * @brief GDK101 방사선 센서 드라이버 - Zephyr용 직접 구현
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gdk101, LOG_LEVEL_INF);

#define GDK101_ADDR         0x18
#define GDK101_REG_STATUS   0x00
#define GDK101_REG_USVH     0x01  /* uSv/h × 100 */
#define GDK101_REG_COUNT    0x02

struct gdk101_data {
    uint16_t usvh_x100;
};

struct gdk101_config {
    struct i2c_dt_spec i2c;
};

int gdk101_read_usvh(const struct device *dev, uint16_t *usvh_x100) {
    const struct gdk101_config *config = dev->config;
    uint8_t reg = GDK101_REG_USVH;
    uint8_t buf[2];
    int ret;
    
    ret = i2c_write_read_dt(&config->i2c, &reg, 1, buf, 2);
    if (ret != 0) {
        return ret;
    }
    
    *usvh_x100 = (buf[0] << 8) | buf[1];
    return 0;
}

int gdk101_init(const struct device *dev) {
    const struct gdk101_config *config = dev->config;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    LOG_INF("GDK101 initialized");
    return 0;
}

#define GDK101_DEFINE(inst) \
    static struct gdk101_data gdk101_data_##inst; \
    static const struct gdk101_config gdk101_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_INST(inst, fuka_gdk101)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, gdk101_init, NULL, \
                          &gdk101_data_##inst, \
                          &gdk101_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(GDK101_DEFINE)
