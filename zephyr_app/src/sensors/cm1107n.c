/**
 * @file cm1107n.c
 * @brief CM1107N CO2 센서 드라이버 - Zephyr용 (STM32Cube 기능 포팅)
 * @details STM32Cube 드라이버에서 다음 기능 포팅:
 *          - 체크섬 검증
 *          - 올바른 프로토콜 구현
 *          - 에러 처리
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cm1107n, LOG_LEVEL_INF);

/* ========================================================================== */
/* 레지스터 정의                                                                */
/* ========================================================================== */

#define CM1107N_ADDR        0x31

/* Command bytes */
#define CM1107N_CMD_READ_CO2 0x11

/* Response format */
#define CM1107N_RESP_HEADER  0x16
#define CM1107N_RESP_LEN     0x05
#define CM1107N_RESP_CMD     0x01

/* ========================================================================== */
/* 데이터 구조                                                                 */
/* ========================================================================== */

struct cm1107n_data {
    uint16_t co2_ppm;
    uint8_t status;
};

struct cm1107n_config {
    struct i2c_dt_spec i2c;
};

/* ========================================================================== */
/* API 함수                                                                    */
/* ========================================================================== */

int cm1107n_read_co2(const struct device *dev, uint16_t *co2_ppm) {
    const struct cm1107n_config *config = dev->config;
    struct cm1107n_data *data = dev->data;
    
    /* Send read command: 0x11 0x01 0x01 0xED */
    uint8_t cmd[4] = { CM1107N_CMD_READ_CO2, 0x01, 0x01, 0xED };
    int ret = i2c_write_dt(&config->i2c, cmd, 4);
    if (ret != 0) {
        LOG_ERR("Failed to send command");
        return ret;
    }
    
    /* Wait for sensor processing (datasheet: ~20ms) */
    k_msleep(20);
    
    /* Read response: 8 bytes
     * Format: Header(0x16) Len(0x05) Cmd(0x01) CO2_H CO2_L Status Checksum
     */
    uint8_t resp[8];
    ret = i2c_read_dt(&config->i2c, resp, 8);
    if (ret != 0) {
        LOG_ERR("Failed to read response");
        return ret;
    }
    
    /* Verify header and length */
    if (resp[0] != CM1107N_RESP_HEADER) {
        LOG_ERR("Invalid header: 0x%02X", resp[0]);
        return -EIO;
    }
    
    if (resp[1] != CM1107N_RESP_LEN) {
        LOG_ERR("Invalid length: 0x%02X", resp[1]);
        return -EIO;
    }
    
    if (resp[2] != CM1107N_RESP_CMD) {
        LOG_ERR("Invalid command response: 0x%02X", resp[2]);
        return -EIO;
    }
    
    /* Verify checksum */
    uint16_t sum = 0;
    for (int i = 0; i < 7; i++) {
        sum += resp[i];
    }
    uint8_t checksum = (256 - (sum % 256)) % 256;
    
    if (checksum != resp[7]) {
        LOG_ERR("Checksum mismatch: calc=0x%02X, recv=0x%02X", checksum, resp[7]);
        return -EIO;
    }
    
    /* Extract CO2 value */
    *co2_ppm = ((uint16_t)resp[3] << 8) | resp[4];
    data->co2_ppm = *co2_ppm;
    data->status = resp[5];
    
    return 0;
}

int cm1107n_get_status(const struct device *dev, uint8_t *status) {
    struct cm1107n_data *data = dev->data;
    *status = data->status;
    return 0;
}

int cm1107n_init(const struct device *dev) {
    const struct cm1107n_config *config = dev->config;
    struct cm1107n_data *data = dev->data;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    /* Initial CO2 read to verify connection */
    uint16_t co2;
    if (cm1107n_read_co2(dev, &co2) != 0) {
        LOG_WRN("Initial read failed, continuing anyway");
    }
    
    data->co2_ppm = 400;  /* Default */
    
    LOG_INF("CM1107N initialized");
    return 0;
}

/* ========================================================================== */
/* Device Tree 인스턴스                                                         */
/* ========================================================================== */

#define CM1107N_DEFINE(inst) \
    static struct cm1107n_data cm1107n_data_##inst; \
    static const struct cm1107n_config cm1107n_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_DRV_INST(inst)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, cm1107n_init, NULL, \
                          &cm1107n_data_##inst, \
                          &cm1107n_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(CM1107N_DEFINE)
