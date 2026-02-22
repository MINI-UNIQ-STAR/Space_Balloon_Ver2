/**
 * @file gdk101.c
 * @brief GDK101 방사선 센서 드라이버 - Zephyr용 (STM32Cube 기능 포팅)
 * @details STM32Cube 드라이버에서 다음 기능 포팅:
 *          - Reset
 *          - Status 읽기 (진동 감지 포함)
 *          - 1분/10분 평균값
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gdk101, LOG_LEVEL_INF);

/* ========================================================================== */
/* 레지스터 정의                                                                */
/* ========================================================================== */

#define GDK101_ADDR         0x18
#define GDK101_REG_STATUS   0x00
#define GDK101_REG_AVG_1MIN 0x01
#define GDK101_REG_AVG_10MIN 0x02
#define GDK101_REG_RESET    0xA0

/* ========================================================================== */
/* 데이터 구조                                                                 */
/* ========================================================================== */

struct gdk101_data {
    float usv_h_1min;
    float usv_h_10min;
    uint8_t status;
    bool vibration;
};

struct gdk101_config {
    struct i2c_dt_spec i2c;
};

/* ========================================================================== */
/* 내부 함수                                                                    */
/* ========================================================================== */

static int gdk101_read_reg(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t len) {
    const struct gdk101_config *config = dev->config;
    int ret;
    
    /* Write register address */
    ret = i2c_write_dt(&config->i2c, &reg, 1);
    if (ret != 0) return ret;
    
    /* Delay per datasheet */
    k_msleep(10);
    
    /* Read data */
    ret = i2c_read_dt(&config->i2c, data, len);
    return ret;
}

/* ========================================================================== */
/* API 함수                                                                    */
/* ========================================================================== */

int gdk101_reset(const struct device *dev) {
    uint8_t buf[2];
    const struct gdk101_config *config = dev->config;
    
    /* Write reset command */
    uint8_t cmd = GDK101_REG_RESET;
    int ret = i2c_write_dt(&config->i2c, &cmd, 1);
    if (ret != 0) return ret;
    
    /* Read response */
    ret = i2c_read_dt(&config->i2c, buf, 2);
    return ret;
}

int gdk101_read_status(const struct device *dev, uint8_t *status, bool *vibration) {
    uint8_t buf[2];
    int ret = gdk101_read_reg(dev, GDK101_REG_STATUS, buf, 2);
    if (ret != 0) return ret;
    
    *status = buf[0];
    *vibration = (buf[1] != 0);
    
    return 0;
}

int gdk101_read_1min_avg(const struct device *dev, float *uSv_h) {
    uint8_t buf[2];
    int ret = gdk101_read_reg(dev, GDK101_REG_AVG_1MIN, buf, 2);
    if (ret != 0) return ret;
    
    /* Value = buf[0] + buf[1] / 100.0 */
    *uSv_h = (float)buf[0] + (float)buf[1] / 100.0f;
    
    struct gdk101_data *data = dev->data;
    data->usv_h_1min = *uSv_h;
    
    return 0;
}

int gdk101_read_10min_avg(const struct device *dev, float *uSv_h) {
    uint8_t buf[2];
    int ret = gdk101_read_reg(dev, GDK101_REG_AVG_10MIN, buf, 2);
    if (ret != 0) return ret;
    
    /* Value = buf[0] + buf[1] / 100.0 */
    *uSv_h = (float)buf[0] + (float)buf[1] / 100.0f;
    
    struct gdk101_data *data = dev->data;
    data->usv_h_10min = *uSv_h;
    
    return 0;
}

int gdk101_read_usvh_x100(const struct device *dev, uint16_t *usvh_x100) {
    float uSv_h;
    int ret = gdk101_read_1min_avg(dev, &uSv_h);
    if (ret != 0) return ret;
    
    *usvh_x100 = (uint16_t)(uSv_h * 100.0f);
    return 0;
}

int gdk101_init(const struct device *dev) {
    const struct gdk101_config *config = dev->config;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    /* Reset sensor */
    if (gdk101_reset(dev) != 0) {
        LOG_WRN("Reset failed, continuing anyway");
    }
    
    k_msleep(100);
    
    /* Check connection via status */
    uint8_t status;
    bool vibration;
    if (gdk101_read_status(dev, &status, &vibration) != 0) {
        LOG_ERR("Failed to read status");
        return -EIO;
    }
    
    LOG_INF("GDK101 initialized (status=0x%02X)", status);
    return 0;
}

/* ========================================================================== */
/* Device Tree 인스턴스                                                         */
/* ========================================================================== */

#define GDK101_DEFINE(inst) \
    static struct gdk101_data gdk101_data_##inst; \
    static const struct gdk101_config gdk101_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_DRV_INST(inst)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, gdk101_init, NULL, \
                          &gdk101_data_##inst, \
                          &gdk101_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(GDK101_DEFINE)
