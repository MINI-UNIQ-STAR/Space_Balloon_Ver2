/**
 * @file sen0321.c
 * @brief SEN0321 오존 센서 드라이버 - Zephyr용 (STM32Cube 기능 포팅)
 * @details DFRobot SEN0321 오존 센서
 *          - I2C 주소: 0x70 (기본) / 0x71
 *          - 자동 모드 지원
 *          - ppb 단위 출력
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sen0321, LOG_LEVEL_INF);

/* ========================================================================== */
/* 레지스터 정의                                                                */
/* ========================================================================== */

#define SEN0321_ADDR_0      0x70
#define SEN0321_ADDR_1      0x71

#define SEN0321_REG_MODE    0x03
#define SEN0321_REG_AUTO_DATA_H 0x09

/* Modes */
#define SEN0321_MODE_PASSIVE  0x00
#define SEN0321_MODE_AUTO     0x01

/* ========================================================================== */
/* 데이터 구조                                                                 */
/* ========================================================================== */

struct sen0321_data {
    int16_t ozone_ppb;
    uint8_t mode;
};

struct sen0321_config {
    struct i2c_dt_spec i2c;
};

/* ========================================================================== */
/* API 함수                                                                    */
/* ========================================================================== */

int sen0321_set_mode(const struct device *dev, uint8_t mode) {
    const struct sen0321_config *config = dev->config;
    struct sen0321_data *data = dev->data;
    
    int ret = i2c_reg_write_byte_dt(&config->i2c, SEN0321_REG_MODE, mode);
    if (ret != 0) {
        LOG_ERR("Failed to set mode");
        return ret;
    }
    
    data->mode = mode;
    return 0;
}

int sen0321_read_ozone(const struct device *dev, int16_t *ozone_ppb) {
    const struct sen0321_config *config = dev->config;
    struct sen0321_data *data = dev->data;
    
    uint8_t buf[2];
    int ret = i2c_burst_read_dt(&config->i2c, SEN0321_REG_AUTO_DATA_H, buf, 2);
    if (ret != 0) {
        LOG_ERR("Failed to read ozone");
        return ret;
    }
    
    *ozone_ppb = ((int16_t)buf[0] << 8) | buf[1];
    data->ozone_ppb = *ozone_ppb;
    
    return 0;
}

int sen0321_init(const struct device *dev) {
    const struct sen0321_config *config = dev->config;
    struct sen0321_data *data = dev->data;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    /* Set to automatic mode */
    int ret = sen0321_set_mode(dev, SEN0321_MODE_AUTO);
    if (ret != 0) {
        LOG_WRN("Failed to set auto mode, continuing anyway");
    }
    
    data->ozone_ppb = 0;
    data->mode = SEN0321_MODE_AUTO;
    
    LOG_INF("SEN0321 initialized");
    return 0;
}

/* ========================================================================== */
/* Device Tree 인스턴스                                                         */
/* ========================================================================== */

#define SEN0321_DEFINE(inst) \
    static struct sen0321_data sen0321_data_##inst; \
    static const struct sen0321_config sen0321_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_DRV_INST(inst)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, sen0321_init, NULL, \
                          &sen0321_data_##inst, \
                          &sen0321_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(SEN0321_DEFINE)
