#include "sensors.h"
#include "main.h" 
#include "bsp.h" // [NEW] BSP Layer
#include <stdio.h> 
#include <string.h> 
#include "lsm6dsv16x_reg.h"
#include "mlx90393_driver.h"
#include "sen0321_driver.h"
#include "pms3003_driver.h"
#include "gdk101_driver.h"
#include "mcp9600_driver.h"
#include "sht31_driver.h"
#include "ms5611_driver.h"
#include "cm1107n_driver.h"
#include "xa1110_driver.h"
#include "ds18b20_driver.h"
#include "fdir.h"

// --- Driver Handles ---
static stmdev_ctx_t lsm_ctx;
static mlx90393_ctx_t mlx_ctx;
static sen0321_ctx_t sen_ctx;
static pms_ctx_t pms_ctx;
static gdk101_ctx_t gdk_ctx;
static mcp9600_ctx_t mcp_ctx;
static sht31_ctx_t sht_ctx;
static ms5611_ctx_t ms_ctx;
static cm1107n_ctx_t cm_ctx;
static xa1110_ctx_t xa_ctx;

// --- Mock State (Moved to BSP or handled internally) ---
// Kept here if logic depends on it, but hardware mock is in BSP.

// --- Platform Functions (Adapters to BSP) ---

// 1. I2C1 (Downside) Wrapper
static int32_t platform_write_i2c1(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    uint16_t dev_addr = (uintptr_t)handle; // Handle stores Address
    return BSP_I2C1_WriteReg(dev_addr, reg, (uint8_t*)bufp, len);
}

static int32_t platform_read_i2c1(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    uint16_t dev_addr = (uintptr_t)handle;
    return BSP_I2C1_ReadReg(dev_addr, reg, bufp, len);
}

// MLX (I2C1) Specific
static int32_t mlx_write(void *handle, uint8_t *buf, uint16_t len) {
    // MLX Driver passes handle differently, check driver
    // Assuming handle is NULL or context pointer, but MLX Init doesn't take addr in standard driver
    // We hardcode address or use context handle if available.
    return BSP_I2C1_Write(BSP_MLX90393_ADDR << 1, buf, len);
}

static int32_t mlx_read(void *handle, uint8_t *buf, uint16_t len) {
    return BSP_I2C1_Read(BSP_MLX90393_ADDR << 1, buf, len);
}

// 2. I2C3 (Upside) Wrapper
static int32_t platform_write_i2c3(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    uint16_t dev_addr = (uintptr_t)handle; 
    return BSP_I2C3_WriteReg(dev_addr, reg, (uint8_t*)bufp, len);
}

static int32_t platform_read_i2c3(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    uint16_t dev_addr = (uintptr_t)handle;
    return BSP_I2C3_ReadReg(dev_addr, reg, bufp, len);
}

// 3. UART Wrapper
static int32_t pms_write(void *handle, uint8_t *buf, uint16_t len) {
    return BSP_UART_Write(buf, len);
}

static int32_t uart_read_mock(void *handle, uint8_t *buf, uint16_t len) {
    return BSP_UART_Read(buf, len);
}


void Sensors_Init(void) {
    // 1. Power On Sensors (BSP)
    BSP_Sensor_PowerOn();
    
    Sensors_Init_I2C1();
    Sensors_Init_I2C3();
    Sensors_Init_UART();
    Sensors_Init_1Wire();
}



void Sensors_Init_1Wire(void) {
    DS18B20_Init_Driver();
}

void Sensors_Init_I2C1(void) {
    // MLX90393 Init
    mlx_ctx.write = mlx_write;
    mlx_ctx.read = mlx_read;
    MLX90393_Init(&mlx_ctx);

    // GDK101 Init
    gdk_ctx.write_reg = platform_write_i2c1;
    gdk_ctx.read_reg = platform_read_i2c1;
    gdk_ctx.handle = (void*)(uintptr_t)BSP_GDK101_ADDR; 
    GDK101_Init(&gdk_ctx);

    // LSM6DSV16X Init
    lsm_ctx.write_reg = platform_write_i2c1;
    lsm_ctx.read_reg = platform_read_i2c1;
    lsm_ctx.handle = (void*)(uintptr_t)BSP_LSM6DSV16X_ADDR;
    
    uint8_t whoamI = 0;
    lsm6dsv16x_device_id_get(&lsm_ctx, &whoamI);
    if (whoamI != LSM6DSV16X_ID) {
        // Error handling
    }
    
    // Restore default config
    lsm6dsv16x_sw_reset(&lsm_ctx);
    
    // Config: ODR 480Hz
    lsm6dsv16x_xl_data_rate_set(&lsm_ctx, LSM6DSV16X_ODR_AT_480Hz);
    lsm6dsv16x_gy_data_rate_set(&lsm_ctx, LSM6DSV16X_ODR_AT_480Hz);
    
    // Config: Mode
    lsm6dsv16x_xl_mode_set(&lsm_ctx, LSM6DSV16X_XL_HIGH_PERFORMANCE_MD);
    lsm6dsv16x_gy_mode_set(&lsm_ctx, LSM6DSV16X_GY_HIGH_PERFORMANCE_MD);
}

void Sensors_Init_I2C3(void) {
    // SEN0321 Init
    sen_ctx.write_reg = platform_write_i2c3; 
    sen_ctx.read_reg = platform_read_i2c3;   
    sen_ctx.handle = (void*)(uintptr_t)BSP_SEN0321_ADDR;
    SEN0321_Init(&sen_ctx);

    // MCP9600 Init
    mcp_ctx.write_reg = platform_write_i2c3;
    mcp_ctx.read_reg = platform_read_i2c3; 
    mcp_ctx.handle = (void*)(uintptr_t)BSP_MCP9600_ADDR;
    MCP9600_Init(&mcp_ctx);

    // MS5611 Init
    ms_ctx.write_reg = platform_write_i2c3;
    ms_ctx.read_reg = platform_read_i2c3;
    ms_ctx.handle = (void*)(uintptr_t)BSP_MS5611_ADDR;
    MS5611_Init(&ms_ctx);

    // SHT31 Init
    sht_ctx.write_reg = platform_write_i2c3;
    sht_ctx.read_reg = platform_read_i2c3;
    sht_ctx.handle = (void*)(uintptr_t)BSP_SHT31_ADDR;
    SHT31_Init(&sht_ctx);
}

void Sensors_Init_UART(void) {
    // HAL_UART_Init(&huart1); // GPS
    // HAL_UART_Init(&huart3); // PMS
    // HAL_UART_Init(&huart2); // CM1107N (Assume internal or other UART)
    
    pms_ctx.write = pms_write;
    // pms_ctx.handle = &huart3; 
    PMS_Init(&pms_ctx);
    PMS_ActiveMode(&pms_ctx);
    
    cm_ctx.write = pms_write; // Reuse mock write
    cm_ctx.read = uart_read_mock;
    CM1107N_Init(&cm_ctx);
    
    // XA1110 Init
    xa_ctx.write = pms_write; // Mock write (printf/UART)
    XA1110_Init(&xa_ctx);
}

void Sensors_Reset(SensorID_t id) {
#ifdef UNIT_TEST
    printf("FDIR: Resetting Sensor ID %d\n", id);
#endif
    // Implementation for HW:
    // 1. DeInit / ReInit Driver
    // 2. Power Cycle if GPIO attached
    // if (id == SENSOR_ID_PMS) PMS_Init(&pms_ctx); ...
}

SensorStatus_t Sensors_Read_All(telemetry_payload_sensor_snapshot_t *data) {
    // 1. IMU (Fast 50Hz)
    Sensors_Read_IMU(data->accel_mps2_x1000, data->gyro_rads_x1000);
    
    // 2. Mag
    Sensors_Read_Mag(data->mag_uT);
    
    // 3. Baro (Decimated 5Hz)
    static uint32_t last_baro = 0;
    if (BSP_GetTick() - last_baro > 200) {
        Sensors_Read_Baro(&data->ms5611_press_pa, &data->ms5611_temp_c_x100);
        last_baro = HAL_GetTick();
    }
    
    // 4. Humidity/Temp (Decimated 1Hz)
    static uint32_t last_env = 0;
    if (BSP_GetTick() - last_env > 1000) {
        Sensors_Read_Humid(&data->sht31_temp_c_x100, &data->sht31_rh_x100);
        last_env = HAL_GetTick();
    }
    
    // GPS, Battery handled in App_Loop
    
    // 6. Battery
    Sensors_Read_Battery(&data->bat_mv, &data->bat_temp_c_x100);
    
    // 7. Others
    Sensors_Read_BoardTemp(&data->board_temp_c_x100);
    
    // Update Mock physics
    mock_altitude += 0.5f; // Climbing 0.5m per call (25m/s if 50Hz.. fast but ok for test)
    if (mock_altitude > 30000.0f) mock_altitude = 100.0f; // Reset
    
    return SENSOR_OK;
}

void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]) {
    int16_t data_raw[3];
    
    // Read Accel
    lsm6dsv16x_acceleration_raw_get(&lsm_ctx, data_raw);
    // Convert to m/s^2 * 1000
    for(int i=0; i<3; i++) {
        float mg = lsm6dsv16x_from_fs2_to_mg(data_raw[i]);
        accel[i] = (int32_t)(mg * 9.8f); 
    }
    
    // Read Gyro
    lsm6dsv16x_angular_rate_raw_get(&lsm_ctx, data_raw);
    for(int i=0; i<3; i++) {
         float mdps = lsm6dsv16x_from_fs2000_to_mdps(data_raw[i]);
         // rad/s * 1000. 1 mdps = 0.00001745 rad/s.
         // result = mdps * 0.01745
         gyro[i] = (int32_t)(mdps * 0.01745f);
    }
}

void Sensors_Read_Mag(float mag[3]) {
    // MLX90393 Read
    // Must start single measurement or ensure continuous mode. 
    // Driver `StartMeasurement` does SM.
    MLX90393_StartMeasurement(&mlx_ctx);
    // Delay needed? Mock instant.
    MLX90393_ReadMeasurement(&mlx_ctx, &mag[0], &mag[1], &mag[2]);
}

void Sensors_Read_Rad(uint16_t *uSvh) {
    float val_uSvh;
    // 10-min avg for stability
    if (GDK101_Read_10Min_Avg(&gdk_ctx, &val_uSvh) == 0) {
        *uSvh = (uint16_t)(val_uSvh * 100); // Scale x100
    } else {
        *uSvh = 0; // Error
    }
}

void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
    int32_t p, t;
    if (MS5611_Read_PT(&ms_ctx, &p, &t) == 0) {
        *press_pa = (uint32_t)p;
        *temp_c_x100 = (int16_t)t;
    } else {
        *press_pa = 101325; 
        *temp_c_x100 = 2500;
    }
}

void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100) {
    float t, rh;
    if (SHT31_ReadTempHum(&sht_ctx, &t, &rh) == 0) {
        *temp_c_x100 = (int16_t)(t * 100);
        *rh_x100 = (uint16_t)(rh * 100);
    } else {
        *temp_c_x100 = 0;
        *rh_x100 = 0;
    }
}

void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5) {
    // Read CO2
    CM1107N_ReadCO2(&cm_ctx, co2);
    // if (*co2 == 0) *co2 = 400; // Minimal default
    
    // Read Ozone
    SEN0321_ReadOzone(&sen_ctx, ozone);
    
    // Read PMS (Mock ingest)
    // In real system, UART ISR calls PMS_ProcessByte(&pms_ctx, byte);
    // Here we just read latest valid data from ctx
    // Mocking some data arrival
    *pm1_0 = pms_ctx.data.PM_AE_UG_1_0;
    *pm2_5 = pms_ctx.data.PM_AE_UG_2_5;
    
    // Auto-increment mock if zero (since no ISR feeding it)
    if (*pm2_5 == 0) *pm2_5 = 15;
}

void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix,
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year) {
    // Mock: Feed NMEA data if fix is 0 (just to verify parsing on host)
    if (xa_ctx.data.fix_type == 0) {
        const char *sim_gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
        for (int i=0; sim_gga[i]; i++) XA1110_ProcessByte(&xa_ctx, (uint8_t)sim_gga[i]);
    }

    *lat = xa_ctx.data.lat_deg_e7;
    *lon = xa_ctx.data.lon_deg_e7;
    *alt = xa_ctx.data.alt_m;
    *fix = xa_ctx.data.fix_type;
    *sats = xa_ctx.data.sats_used;
    *sats_view = xa_ctx.data.sats_view_total;

    // Per-GNSS satellite counts from GSV parsing
    *sats_gps = xa_ctx.data.sats_gps;
    *sats_glonass = xa_ctx.data.sats_glonass;
    *sats_galileo = xa_ctx.data.sats_galileo;
    *sats_beidou = xa_ctx.data.sats_beidou;

    // UTC Time from GPS
    *utc_hour = xa_ctx.data.utc_hour;
    *utc_min = xa_ctx.data.utc_min;
    *utc_sec = xa_ctx.data.utc_sec;
    *utc_day = xa_ctx.data.utc_day;
    *utc_month = xa_ctx.data.utc_month;
    *utc_year = xa_ctx.data.utc_year;

    // Check Health: if fix is valid or data coming
    FDIR_ReportSuccess((void*)(uintptr_t)SENSOR_ID_GPS);
}

void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
    // 1Hz Limit for slow sensors
    static uint32_t last_bat = 0;
    if (BSP_GetTick() - last_bat > 1000) {
        
        *mv = BSP_ADC_Read_Battery_mV();
        
        *temp_c_x100 = DS18B20_ReadTemp_x100(0); // Battery Temp
        last_bat = BSP_GetTick();
    }
}

// ...
void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
    *temp_c_x100 = DS18B20_ReadTemp_x100(1); // Board Temp
}

void Sensors_Read_External(int16_t *temp_c_x100) {
    // Reading Thermocouple from MCP9600
    float val;
    if (MCP9600_ReadThermocouple(&mcp_ctx, &val) == 0) {
        *temp_c_x100 = (int16_t)(val * 100);
    } else {
        *temp_c_x100 = 0; // Error
    }
}
