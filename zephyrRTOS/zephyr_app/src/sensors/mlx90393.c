/**
 * @file mlx90393.c
 * @brief MLX90393 3축 자기계 드라이버 - Zephyr용 (STM32Cube 기능 포팅)
 * @details STM32Cube 드라이버에서 다음 기능 포팅:
 *          - Gain/Resolution/Oversampling/Filter 설정
 *          - Lookup 테이블을 이용한 정확한 변환
 *          - 레지스터 읽기/쓰기
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(mlx90393, LOG_LEVEL_INF);

/* ========================================================================== */
/* 레지스터 정의                                                                */
/* ========================================================================== */

#define MLX90393_ADDR           0x0C
#define MLX90393_AXIS_ALL       0x0E

#define MLX90393_REG_SB         0x10  /* Start Burst */
#define MLX90393_REG_SW         0x20  /* Start Wake-up */
#define MLX90393_REG_SM         0x30  /* Start Single Measurement */
#define MLX90393_REG_RM         0x40  /* Read Measurement */
#define MLX90393_REG_RR         0x50  /* Read Register */
#define MLX90393_REG_WR         0x60  /* Write Register */
#define MLX90393_REG_EX         0x80  /* Exit */
#define MLX90393_REG_RT         0xF0  /* Reset */

#define MLX90393_CONF1          0x00
#define MLX90393_CONF2          0x01
#define MLX90393_CONF3          0x02

/* ========================================================================== */
/* 열거형                                                                      */
/* ========================================================================== */

typedef enum {
    MLX90393_GAIN_5X = 0,
    MLX90393_GAIN_4X,
    MLX90393_GAIN_3X,
    MLX90393_GAIN_2_5X,
    MLX90393_GAIN_2X,
    MLX90393_GAIN_1_67X,
    MLX90393_GAIN_1_33X,
    MLX90393_GAIN_1X
} mlx90393_gain_t;

typedef enum {
    MLX90393_RES_16 = 0,
    MLX90393_RES_17,
    MLX90393_RES_18,
    MLX90393_RES_19,
} mlx90393_resolution_t;

typedef enum {
    MLX90393_OSR_0 = 0,
    MLX90393_OSR_1,
    MLX90393_OSR_2,
    MLX90393_OSR_3,
} mlx90393_oversampling_t;

typedef enum {
    MLX90393_FILTER_0 = 0,
    MLX90393_FILTER_1,
    MLX90393_FILTER_2,
    MLX90393_FILTER_3,
    MLX90393_FILTER_4,
    MLX90393_FILTER_5,
    MLX90393_FILTER_6,
    MLX90393_FILTER_7,
} mlx90393_filter_t;

/* ========================================================================== */
/* Lookup 테이블 (STM32Cube에서 포팅)                                          */
/* ========================================================================== */

/* LSB -> µT 변환 테이블 [gain][resolution][0=XY, 1=Z] */
static const float mlx90393_lsb_lookup[8][4][2] = {
    {{0.751f, 1.210f}, {1.502f, 2.420f}, {3.004f, 4.840f}, {6.009f, 9.680f}},  /* 5x */
    {{0.601f, 0.968f}, {1.202f, 1.936f}, {2.403f, 3.872f}, {4.840f, 7.744f}},  /* 4x */
    {{0.451f, 0.726f}, {0.901f, 1.452f}, {1.803f, 2.904f}, {3.605f, 5.808f}},  /* 3x */
    {{0.376f, 0.605f}, {0.751f, 1.210f}, {1.502f, 2.420f}, {3.004f, 4.840f}},  /* 2.5x */
    {{0.300f, 0.484f}, {0.601f, 0.968f}, {1.202f, 1.936f}, {2.403f, 3.872f}},  /* 2x */
    {{0.250f, 0.403f}, {0.501f, 0.807f}, {1.001f, 1.613f}, {2.003f, 3.227f}},  /* 1.6x */
    {{0.200f, 0.323f}, {0.401f, 0.645f}, {0.801f, 1.291f}, {1.602f, 2.581f}},  /* 1.3x */
    {{0.150f, 0.242f}, {0.300f, 0.484f}, {0.601f, 0.968f}, {1.202f, 1.936f}}   /* 1x */
};

/* ========================================================================== */
/* 데이터 구조                                                                 */
/* ========================================================================== */

struct mlx90393_data {
    float mag_x;
    float mag_y;
    float mag_z;
    mlx90393_gain_t gain;
    mlx90393_resolution_t res_x;
    mlx90393_resolution_t res_y;
    mlx90393_resolution_t res_z;
    mlx90393_oversampling_t osr;
    mlx90393_filter_t dig_filt;
};

struct mlx90393_config {
    struct i2c_dt_spec i2c;
};

/* ========================================================================== */
/* 내부 함수                                                                    */
/* ========================================================================== */

static int mlx90393_write(const struct device *dev, const uint8_t *data, uint16_t len) {
    const struct mlx90393_config *config = dev->config;
    return i2c_write_dt(&config->i2c, data, len);
}

static int mlx90393_read(const struct device *dev, uint8_t *data, uint16_t len) {
    const struct mlx90393_config *config = dev->config;
    return i2c_read_dt(&config->i2c, data, len);
}

static int mlx90393_write_reg(const struct device *dev, uint8_t reg, uint16_t data) {
    uint8_t tx[4] = {
        MLX90393_REG_WR,
        (uint8_t)(data >> 8),
        (uint8_t)(data & 0xFF),
        (uint8_t)(reg << 2)
    };
    
    if (mlx90393_write(dev, tx, 4) != 0) return -1;
    
    uint8_t stat;
    if (mlx90393_read(dev, &stat, 1) != 0) return -1;
    
    return 0;
}

static int mlx90393_read_reg(const struct device *dev, uint8_t reg, uint16_t *data) {
    uint8_t tx[2] = { MLX90393_REG_RR, (uint8_t)(reg << 2) };
    
    if (mlx90393_write(dev, tx, 2) != 0) return -1;
    
    uint8_t rx[3];
    if (mlx90393_read(dev, rx, 3) != 0) return -1;
    
    *data = ((uint16_t)rx[1] << 8) | rx[2];
    return 0;
}

/* ========================================================================== */
/* API 함수                                                                    */
/* ========================================================================== */

int mlx90393_set_gain(const struct device *dev, mlx90393_gain_t gain) {
    struct mlx90393_data *data = dev->data;
    uint16_t reg_data;
    
    if (mlx90393_read_reg(dev, MLX90393_CONF1, &reg_data) != 0) return -1;
    
    reg_data &= ~0x0070;
    reg_data |= (gain << 4);
    
    if (mlx90393_write_reg(dev, MLX90393_CONF1, reg_data) != 0) return -1;
    
    data->gain = gain;
    return 0;
}

int mlx90393_set_resolution(const struct device *dev, uint8_t axis, mlx90393_resolution_t res) {
    struct mlx90393_data *data = dev->data;
    uint16_t reg_data;
    
    if (mlx90393_read_reg(dev, MLX90393_CONF3, &reg_data) != 0) return -1;
    
    switch (axis) {
        case 0: /* X */
            data->res_x = res;
            reg_data &= ~0x0060;
            reg_data |= (res << 5);
            break;
        case 1: /* Y */
            data->res_y = res;
            reg_data &= ~0x0180;
            reg_data |= (res << 7);
            break;
        case 2: /* Z */
            data->res_z = res;
            reg_data &= ~0x0600;
            reg_data |= (res << 9);
            break;
    }
    
    return mlx90393_write_reg(dev, MLX90393_CONF3, reg_data);
}

int mlx90393_set_oversampling(const struct device *dev, mlx90393_oversampling_t osr) {
    struct mlx90393_data *data = dev->data;
    uint16_t reg_data;
    
    if (mlx90393_read_reg(dev, MLX90393_CONF3, &reg_data) != 0) return -1;
    
    reg_data &= ~0x03;
    reg_data |= osr;
    
    if (mlx90393_write_reg(dev, MLX90393_CONF3, reg_data) != 0) return -1;
    
    data->osr = osr;
    return 0;
}

int mlx90393_set_filter(const struct device *dev, mlx90393_filter_t filter) {
    struct mlx90393_data *data = dev->data;
    uint16_t reg_data;
    
    if (mlx90393_read_reg(dev, MLX90393_CONF3, &reg_data) != 0) return -1;
    
    reg_data &= ~0x1C;
    reg_data |= (filter << 2);
    
    if (mlx90393_write_reg(dev, MLX90393_CONF3, reg_data) != 0) return -1;
    
    data->dig_filt = filter;
    return 0;
}

int mlx90393_reset(const struct device *dev) {
    uint8_t tx = MLX90393_REG_RT;
    
    if (mlx90393_write(dev, &tx, 1) != 0) return -1;
    
    uint8_t stat;
    if (mlx90393_read(dev, &stat, 1) != 0) return -1;
    
    return 0;
}

int mlx90393_sample_fetch(const struct device *dev) {
    struct mlx90393_data *data = dev->data;
    
    /* Start measurement */
    uint8_t tx = MLX90393_REG_SM | MLX90393_AXIS_ALL;
    if (mlx90393_write(dev, &tx, 1) != 0) return -1;
    
    uint8_t stat;
    if (mlx90393_read(dev, &stat, 1) != 0) return -1;
    
    /* Wait for measurement */
    k_msleep(10);
    
    /* Read measurement */
    tx = MLX90393_REG_RM | MLX90393_AXIS_ALL;
    if (mlx90393_write(dev, &tx, 1) != 0) return -1;
    
    uint8_t rx[7];
    if (mlx90393_read(dev, rx, 7) != 0) return -1;
    
    int16_t xi = (rx[1] << 8) | rx[2];
    int16_t yi = (rx[3] << 8) | rx[4];
    int16_t zi = (rx[5] << 8) | rx[6];
    
    /* Apply conversion using lookup table */
    data->mag_x = (float)xi * mlx90393_lsb_lookup[data->gain][data->res_x][0];
    data->mag_y = (float)yi * mlx90393_lsb_lookup[data->gain][data->res_y][0];
    data->mag_z = (float)zi * mlx90393_lsb_lookup[data->gain][data->res_z][1];
    
    return 0;
}

int mlx90393_channel_get(const struct device *dev, float mag[3]) {
    struct mlx90393_data *data = dev->data;
    
    mag[0] = data->mag_x;
    mag[1] = data->mag_y;
    mag[2] = data->mag_z;
    
    return 0;
}

int mlx90393_init(const struct device *dev) {
    const struct mlx90393_config *config = dev->config;
    struct mlx90393_data *data = dev->data;
    
    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    
    /* Exit mode */
    uint8_t tx = MLX90393_REG_EX;
    mlx90393_write(dev, &tx, 1);
    uint8_t stat;
    mlx90393_read(dev, &stat, 1);
    
    /* Reset */
    mlx90393_reset(dev);
    k_msleep(10);
    
    /* Default settings (matching Adafruit) */
    data->gain = MLX90393_GAIN_1X;
    data->res_x = MLX90393_RES_16;
    data->res_y = MLX90393_RES_16;
    data->res_z = MLX90393_RES_16;
    data->osr = MLX90393_OSR_3;
    data->dig_filt = MLX90393_FILTER_7;
    
    mlx90393_set_gain(dev, MLX90393_GAIN_1X);
    mlx90393_set_resolution(dev, 0, MLX90393_RES_16);
    mlx90393_set_resolution(dev, 1, MLX90393_RES_16);
    mlx90393_set_resolution(dev, 2, MLX90393_RES_16);
    mlx90393_set_oversampling(dev, MLX90393_OSR_3);
    mlx90393_set_filter(dev, MLX90393_FILTER_7);
    
    LOG_INF("MLX90393 initialized");
    return 0;
}

/* ========================================================================== */
/* Device Tree 인스턴스                                                         */
/* ========================================================================== */

#define MLX90393_DEFINE(inst) \
    static struct mlx90393_data mlx90393_data_##inst = { \
        .gain = MLX90393_GAIN_1X, \
        .res_x = MLX90393_RES_16, \
        .res_y = MLX90393_RES_16, \
        .res_z = MLX90393_RES_16, \
        .osr = MLX90393_OSR_3, \
        .dig_filt = MLX90393_FILTER_7, \
    }; \
    static const struct mlx90393_config mlx90393_config_##inst = { \
        .i2c = I2C_DT_SPEC_GET(DT_DRV_INST(inst)), \
    }; \
    DEVICE_DT_INST_DEFINE(inst, mlx90393_init, NULL, \
                          &mlx90393_data_##inst, \
                          &mlx90393_config_##inst, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(MLX90393_DEFINE)
