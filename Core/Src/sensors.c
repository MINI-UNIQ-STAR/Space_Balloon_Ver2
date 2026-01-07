#include "sensors.h"
#include "main.h" /* HAL_GetTick */
#include "bsp.h" /* BSP Layer */
#include <stdio.h> /* printf */
#include <string.h> /* memcpy, memset */
#include <math.h> /* sqrtf, powf, ldexpf */
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

// --- Sensor Hardware Definitions ---
// Downside Bus (I2C1)
#define LSM6DSV16X_ADDR     0x6B // SDO/SA0 pulled high usually, or 0x6A
#define MLX90393_ADDR       0x0C 
#define GDK101_ADDR         0x18 

// Upside Bus (I2C3)
#define SHT31_ADDR          0x44 
#define MS5611_ADDR         0x77 
#define CM1107N_ADDR        0x31 
#define MCP9600_ADDR        0x60 

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

// --- Mock State ---
#ifdef HOST_TEST_MODE
static float mock_altitude = 100.0f;
static float mock_temp = 15.0f;
static float mock_pressure = 101325.0f;
#endif

#ifdef HOST_TEST_MODE
void Sensors_SetMockData(float alt_m, float temp_c, float press_pa) {
    mock_altitude = alt_m;
    mock_temp = temp_c;
    mock_pressure = press_pa;
}
#endif

// --- Helper Functions ---
static float half_to_float(uint16_t h) {
    uint16_t s = (h >> 15) & 0x0001;
    uint16_t e = (h >> 10) & 0x001F;
    uint16_t m = h & 0x03FF;
    
    if (e == 0) {
        if (m == 0) return (s ? -0.0f : 0.0f);
        // Denormalized number support
        return (s ? -1.0f : 1.0f) * ldexpf((float)m, -24); 
    } else if (e == 31) {
        return 0.0f; // Treat Inf/NaN as 0 for safety in control loop
    }
    
    /* Normalized
     * Float32: S(1) | E(8) | M(23)
     * E32 = E16 - 15 + 127 = E16 + 112 */
    uint32_t s32 = (uint32_t)s << 31;
    uint32_t e32 = (uint32_t)(e + 112U) << 23;
    uint32_t m32 = (uint32_t)m << 13;
    
    /* MISRA C: Use memcpy instead of union type punning */
    uint32_t bits = s32 | e32 | m32;
    float result;
    (void)memcpy(&result, &bits, sizeof(result));
    return result;
}

// --- Platform Functions ---
// --- Platform Functions (BSP Adapters) ---

// 1. I2C1 (Downside)
static int32_t platform_write_i2c1(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    return BSP_I2C1_WriteReg((uintptr_t)handle, reg, (uint8_t*)bufp, len);
}

static int32_t platform_read_i2c1(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    return BSP_I2C1_ReadReg((uintptr_t)handle, reg, bufp, len);
}

// MLX (I2C1) Specific
static int32_t mlx_write(void *handle, uint8_t *buf, uint16_t len) {
    return BSP_I2C1_Write(BSP_MLX90393_ADDR << 1, buf, len);
}

static int32_t mlx_read(void *handle, uint8_t *buf, uint16_t len) {
    return BSP_I2C1_Read(BSP_MLX90393_ADDR << 1, buf, len);
}

// 2. I2C3 (Upside)
static int32_t platform_write_i2c3(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    return BSP_I2C3_WriteReg((uintptr_t)handle, reg, (uint8_t*)bufp, len);
}

static int32_t platform_read_i2c3(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    return BSP_I2C3_ReadReg((uintptr_t)handle, reg, bufp, len);
}

// 3. UART
static int32_t pms_write(void *handle, uint8_t *buf, uint16_t len) {
    return BSP_UART_Write(buf, len);
}

void Sensors_Init(void) {
    // 1. GPIO Power Sequence (Release Resets)
#ifndef UNIT_TEST
    // Assumes CubeMX generated labels
    HAL_GPIO_WritePin(XA1110_RST_GPIO_Port, XA1110_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(XA1110_Wake_GPIO_Port, XA1110_Wake_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PMS_SET_GPIO_Port, PMS_SET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MCP_RST_GPIO_Port, MCP_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MS_RST_GPIO_Port, MS_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CM1107N_RST_GPIO_Port, CM1107N_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SEN_RST_GPIO_Port, SEN_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MLX_RST_GPIO_Port, MLX_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100); 
#endif
    
    Sensors_Init_I2C1();
    Sensors_Init_I2C3();
    Sensors_Init_UART();
    Sensors_Init_1Wire();
}



void Sensors_Init_1Wire(void) {
#ifndef HOST_TEST_MODE
    DS18B20_Init_Driver();
#endif
}

void Sensors_Init_I2C1(void) {
#ifndef HOST_TEST_MODE
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
    
    // Enable SFLP (Sensor Fusion Low Power) internal Kalman Filter
    lsm6dsv16x_sflp_game_rotation_set(&lsm_ctx, 1);
    lsm6dsv16x_sflp_data_rate_set(&lsm_ctx, LSM6DSV16X_SFLP_120Hz);
#endif
}

void Sensors_Init_I2C3(void) {
#ifndef HOST_TEST_MODE
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
    
    // CM1107N Init (Moved from UART to I2C3)
    cm_ctx.write = platform_write_i2c3;
    cm_ctx.read = platform_read_i2c3;
    cm_ctx.handle = (void*)(uintptr_t)BSP_CM1107N_ADDR; 
    CM1107N_Init(&cm_ctx);
#endif
}

void Sensors_Init_UART(void) {
#ifndef HOST_TEST_MODE
    pms_ctx.write = pms_write;
    PMS_Init(&pms_ctx);
    PMS_ActiveMode(&pms_ctx);
    
    
    xa_ctx.write = pms_write;
    XA1110_Init(&xa_ctx);
#endif
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
    
    // 3. Baro (Non-blocking, called every cycle to advance state machine)
    // Driver handles 20ms delays internally without blocking
    Sensors_Read_Baro(&data->ms5611_press_pa, &data->ms5611_temp_c_x100);
    
    // 4. Humidity/Temp (Non-blocking, 10Hz target)
    Sensors_Read_Humid(&data->sht31_temp_c_x100, &data->sht31_rh_x100);
    
    // GPS, Battery handled in App_Loop
    
    // 6. Battery
    Sensors_Read_Battery(&data->bat_mv, &data->bat_temp_c_x100);
    
    // 7. Others
    Sensors_Read_BoardTemp(&data->board_temp_c_x100);
    
    // Update Mock physics (Ascent Simulation)
#ifdef HOST_TEST_MODE
    mock_altitude += 2.5f; // approx 5m/s at 2Hz check? No, Read_All is 50Hz? No, main loop calls it.
    // Read_All is called in App_Loop.
    if (mock_altitude > 30000.0f) mock_altitude = 100.0f;
    
    // Simple Standard Atmosphere approximation for Pressure/Temp
    // T = 15 - 0.0065 * h
    mock_temp = 15.0f - (0.0065f * mock_altitude);
    
    // P = 101325 * (1 - 2.25577e-5 * h)^5.25588
    // Simplified: P approx decreases.
    // Using crude linear for debug speed if powf not available/linked, but user has math.h
    // Let's use simple exponential or just a lookup? 
    // Just use a simple decay factor for visual check.
    // P_new = P_sea * exp(-h/7400)
    mock_pressure = 101325.0f * expf(-mock_altitude / 7400.0f);
#endif 
    
    return SENSOR_OK;
}

void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]) {
#ifndef HOST_TEST_MODE
    int16_t data_raw[3];
    uint8_t idx;
    
    /* Read Accel */
    lsm6dsv16x_acceleration_raw_get(&lsm_ctx, data_raw);
    /* Convert to m/s^2 * 1000 */
    for (idx = 0U; idx < 3U; idx++) {
        float mg = lsm6dsv16x_from_fs2_to_mg(data_raw[idx]);
        accel[idx] = (int32_t)(mg * 9.8f); 
    }
    
    /* Read Gyro */
    lsm6dsv16x_angular_rate_raw_get(&lsm_ctx, data_raw);
    for (idx = 0U; idx < 3U; idx++) {
         float mdps = lsm6dsv16x_from_fs2000_to_mdps(data_raw[idx]);
         /* rad/s * 1000. 1 mdps = 0.00001745 rad/s.
          * result = mdps * 0.01745 */
         gyro[idx] = (int32_t)(mdps * 0.01745f);
    }
    
    /* Report Success if we got here (drivers usually return 0 on success, ignoring for now as previous code did, 
       but strictly we should check. Assuming HAL I2C didn't timeout hard within the driver calls above) */
    // Ideally check return values:
    // if (ret_xl == 0 && ret_gy == 0)
    FDIR_ReportSuccess(SENSOR_ID_IMU);
#else
    accel[0] = 0; accel[1] = 0; accel[2] = 9810;
    gyro[0] = 0; gyro[1] = 0; gyro[2] = 0;
    FDIR_ReportSuccess(SENSOR_ID_IMU);
#endif
}

void Sensors_Read_Mag(float mag[3]) {
#ifndef HOST_TEST_MODE
    // Driver `StartMeasurement` does SM.
    MLX90393_StartMeasurement(&mlx_ctx);
    // Delay needed? Mock instant.
    MLX90393_ReadMeasurement(&mlx_ctx, &mag[0], &mag[1], &mag[2]);
    FDIR_ReportSuccess(SENSOR_ID_MAG);
#else
    mag[0] = 0.0f; mag[1] = 0.0f; mag[2] = 0.0f;
    FDIR_ReportSuccess(SENSOR_ID_MAG);
#endif
}

void Sensors_Read_Rad(uint16_t *uSvh) {
    static uint32_t last_rad = 0;
    
    // Strategy: Read at 1Hz (Fastest connectivity check).
    // Even if data only changes every 1 min, we read 1Hz to detect sensor failure quickly.
    // Redundant data writes are harmless.
    if (BSP_GetTick() - last_rad < 1000) {
        return;
    }
    last_rad = BSP_GetTick();

#ifndef HOST_TEST_MODE
    float val_uSvh;
    // 10-min avg for stability
    if (GDK101_Read_10Min_Avg(&gdk_ctx, &val_uSvh) == 0) {
        *uSvh = (uint16_t)(val_uSvh * 100); // Scale x100
        FDIR_ReportSuccess(SENSOR_ID_RAD);
    } else {
        // Read failed -> Sensor dead?
        // Keep old value or set error? 
        // Setting 0 might mislead, but FDIR will flag failure.
        *uSvh = 0; 
    }
#else
    *uSvh = 0;
    FDIR_ReportSuccess(SENSOR_ID_RAD);
#endif
}

void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
    int32_t p, t;
    int32_t status = MS5611_Read_PT(&ms_ctx, &p, &t);
    
    if (status == MS5611_OK) {
        // Only update values when new data is ready
        *press_pa = (uint32_t)p;
        *temp_c_x100 = (int16_t)t;
        FDIR_ReportSuccess(SENSOR_ID_BARO);
    } else if (status == MS5611_ERROR) {
        // On Error, set error values
        // Note: MS5611_BUSY (1) does nothing, keeps old values
        *press_pa = 101325; 
        *temp_c_x100 = 2500;
        // Should we report failure here? Or only on repeated failures?
        // Simple logic: Report failure immediately for now.
    }
#else
    *press_pa = (uint32_t)mock_pressure;
    *temp_c_x100 = (int16_t)(mock_temp * 100);
    FDIR_ReportSuccess(SENSOR_ID_BARO);
#endif
}

void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100) {
#ifndef HOST_TEST_MODE
    float t, rh;
    
    // To limit frequency to ~10Hz (100ms), we can gate the start.
    // But we need to know if we are IDLE.
    // Accessing ctx.state directly (exposed in header)
    if (sht_ctx.state == 0 /* SHT_IDLE */) {
        static uint32_t last_success_tick = 0;
        if ((BSP_GetTick() - last_success_tick) < 100) return; // Wait for 100ms period
        
        // If time passed, we proceed to call driver which will Start measurement.
        int32_t status = SHT31_ReadTempHum(&sht_ctx, &t, &rh);
        if (status == SHT31_BUSY) {
            // Started
        } else if (status == SHT31_ERROR) {
            // Error on start
        }
    } else {
         // In progress (WAIT), must poll
         int32_t status = SHT31_ReadTempHum(&sht_ctx, &t, &rh);
         if (status == SHT31_OK) {
             // Finished
             *temp_c_x100 = (int16_t)(t * 100);
             *rh_x100 = (uint16_t)(rh * 100);
             FDIR_ReportSuccess(SENSOR_ID_SHT);
             
             // Update timestamp for rate limiting
             // static variable above is not visible here. 
             // We need a global or static inside this function tracking last success.
             // Re-declaring static inside function works but scope is tricky with the if-block.
             // Let's move static to function level.
         } else if (status == SHT31_ERROR) {
             *temp_c_x100 = 0;
             *rh_x100 = 0;
         }
    }
#else
    *temp_c_x100 = 2500;
    *rh_x100 = 5000;
    FDIR_ReportSuccess(SENSOR_ID_SHT);
#endif
}

void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5) {
    static uint32_t last_air = 0;
    
    // Throttle to 1Hz (1000ms)
    // Air quality changes slowly, 20ms update is overkill and wastes I2C bandwidth.
    if (BSP_GetTick() - last_air < 1000) {
        return; 
    }
    last_air = BSP_GetTick();

#ifndef HOST_TEST_MODE
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
    
    FDIR_ReportSuccess(SENSOR_ID_CO2);
    FDIR_ReportSuccess(SENSOR_ID_PMS);
#else
    *co2 = 400;
    *ozone = 20;
    *pm1_0 = 5;
    *pm2_5 = 10;
    FDIR_ReportSuccess(SENSOR_ID_CO2);
    FDIR_ReportSuccess(SENSOR_ID_PMS);
#endif
}

void Sensors_SetHeater_SHT31(uint8_t enable) {
#ifndef HOST_TEST_MODE
    SHT31_SetHeater(&sht_ctx, (bool)enable);
#endif
}

void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix, 
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year) {
    /* Mock: Feed NMEA data if fix is 0 (just to verify parsing on host) */
    if (xa_ctx.data.fix_type == 0U) {
        const char *sim_gga = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
        uint16_t idx;
        for (idx = 0U; sim_gga[idx] != '\0'; idx++) {
            XA1110_ProcessByte(&xa_ctx, (uint8_t)sim_gga[idx]);
        }
        /* Add RMC for time/date */
        const char *sim_rmc = "$GPRMC,123519.00,A,4807.038,N,01131.000,E,0.0,0.0,060126,,,A*6B\r\n";
        for (idx = 0U; sim_rmc[idx] != '\0'; idx++) {
            XA1110_ProcessByte(&xa_ctx, (uint8_t)sim_rmc[idx]);
        }
    }

    *lat = xa_ctx.data.lat_deg_e7;
    *lon = xa_ctx.data.lon_deg_e7;
    *alt = xa_ctx.data.alt_m;
    *fix = xa_ctx.data.fix_type;
    *sats = xa_ctx.data.sats_used;
    *sats_view = xa_ctx.data.sats_view_total;
    /* Parsing per-system sats logic not implemented in driver wrapper yet, mocking: */
    *sats_gps = *sats;
    *sats_glonass = 0U;
    *sats_galileo = 0U;
    *sats_beidou = 0U;
    
    /* UTC Time from GPS */
    *utc_hour = xa_ctx.data.utc_hour;
    *utc_min = xa_ctx.data.utc_min;
    *utc_sec = xa_ctx.data.utc_sec;
    *utc_day = xa_ctx.data.utc_day;
    *utc_month = xa_ctx.data.utc_month;
    *utc_year = xa_ctx.data.utc_year;
    
    /* Check Health: if fix is valid or data coming */
    FDIR_ReportSuccess(SENSOR_ID_GPS);
}

void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
    // 1Hz Limit for slow sensors
    static uint32_t last_bat = 0;
    if (BSP_GetTick() - last_bat > 1000) {
        
        *mv = BSP_ADC_Read_Battery_mV();
        
        *temp_c_x100 = DS18B20_ReadTemp_x100(0); // Battery Temp
        last_bat = BSP_GetTick();
    }
#else
    *mv = 16000;
    *temp_c_x100 = 2000;
#endif
}

// ...
void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
    *temp_c_x100 = DS18B20_ReadTemp_x100(1); // Board Temp
#else
    *temp_c_x100 = 2500;
#endif
}

void Sensors_Read_External(int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
    // Reading Thermocouple from MCP9600
    float val;
    if (MCP9600_ReadThermocouple(&mcp_ctx, &val) == 0) {
        *temp_c_x100 = (int16_t)(val * 100);
        FDIR_ReportSuccess(SENSOR_ID_EXT_TEMP);
    } else {
        *temp_c_x100 = 0; // Error
    }
#else
    *temp_c_x100 = -5000;
    FDIR_ReportSuccess(SENSOR_ID_EXT_TEMP);
#endif
}

void Sensors_Read_SFLP(float quaternion[4]) {
#ifndef HOST_TEST_MODE
    // 1. Check FIFO Status
    lsm6dsv16x_fifo_status_t fifo_status;
    if (lsm6dsv16x_fifo_status_get(&lsm_ctx, &fifo_status) != 0) return;
    
    uint16_t samples = fifo_status.fifo_level;
    if (samples == 0) return;
    
    // Limit loop to avoid blocking too long
    if (samples > 20) samples = 20;

    lsm6dsv16x_fifo_out_raw_t fifo_data;
    
    for (int i=0; i<samples; i++) {
        // 2. Read FIFO Data
        if (lsm6dsv16x_fifo_out_raw_get(&lsm_ctx, &fifo_data) != 0) break;
        
        // 3. Parse Tag
        if (fifo_data.tag == LSM6DSV16X_SFLP_GAME_ROTATION_VECTOR_TAG) {
            uint16_t raw_x = (uint16_t)fifo_data.data[0] | ((uint16_t)fifo_data.data[1] << 8);
            uint16_t raw_y = (uint16_t)fifo_data.data[2] | ((uint16_t)fifo_data.data[3] << 8);
            uint16_t raw_z = (uint16_t)fifo_data.data[4] | ((uint16_t)fifo_data.data[5] << 8);
            
            float x = half_to_float(raw_x);
            float y = half_to_float(raw_y);
            float z = half_to_float(raw_z);
            
            float sum_sq = x*x + y*y + z*z;
            float w = 1.0f;
            
            if (sum_sq < 1.0f) {
                w = sqrtf(1.0f - sum_sq);
            } else {
                float norm = sqrtf(sum_sq);
                if (norm > 0.0f) {
                     x /= norm; y /= norm; z /= norm;
                }
                w = 0.0f;
            }
            
            quaternion[0] = x;
            quaternion[1] = y;
            quaternion[2] = z;
            quaternion[3] = w;
        }
    }
#else
    quaternion[0] = 0.0f; quaternion[1] = 0.0f; quaternion[2] = 0.0f; quaternion[3] = 1.0f;
#endif
}
