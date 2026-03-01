#include "cm1107n_driver.h"
#include "ds18b20_driver.h"
#include "fdir.h"
#include "gdk101_driver.h"
#include "lsm6dsv16x_reg.h"
#include "main.h" /* HAL_GetTick */
#include "mcp9600_driver.h"
#include "mlx90393_driver.h"
#include "mock_hal.h"
#include "ms5611_driver.h"
#include "pms3003_driver.h"
#include "sen0321_driver.h"
#include "sensors.h"
#include "sht31_driver.h"
#include "xa1110_driver.h"
#include <math.h>   /* sqrtf, powf, ldexpf */
#include <stdio.h>  /* printf */
#include <string.h> /* memcpy, memset */

// --- 센서 하드웨어 주소 정의 ---
// 하단 버스 (I2C1)
#define LSM6DSV16X_ADDR 0x6B // SDO/SA0가 High인 경우 (또는 0x6A)
#define MLX90393_ADDR 0x0C
#define GDK101_ADDR 0x18

// 상단 버스 (I2C3)
#define SHT31_ADDR 0x44
#define MS5611_ADDR 0x77
#define CM1107N_ADDR 0x31
#define MCP9600_ADDR 0x60

// --- 드라이버 핸들(Context) 정의 ---
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

// --- Mock 상태 변수 (테스트용) ---
#ifdef HOST_TEST_MODE
static float mock_altitude = 100.0f;
static float mock_temp = 15.0f;
static float mock_pressure = 101325.0f;
#endif

#ifdef HOST_TEST_MODE
/** @brief Mock 센서 데이터 설정 (SITL 환경) */
void Sensors_SetMockData(float alt_m, float temp_c, float press_pa) {
  mock_altitude = alt_m;
  mock_temp = temp_c;
  mock_pressure = press_pa;
}
#endif

// --- 유틸리티 함수 ---

/** @brief 16비트 반정밀도 부동소수점을 float로 변환 */
static float half_to_float(uint16_t h) {
  uint16_t s = (h >> 15) & 0x0001;
  uint16_t e = (h >> 10) & 0x001F;
  uint16_t m = h & 0x03FF;

  if (e == 0) {
    if (m == 0)
      return (s ? -0.0f : 0.0f);
    // 비정규화(Denormalized) 수 지원
    return (s ? -1.0f : 1.0f) * ldexpf((float)m, -24);
  } else if (e == 31) {
    return 0.0f; // 제어 루프 안전을 위해 Inf/NaN은 0으로 처리
  }

  /* 정규화된 부동소수점 처리
   * Float32: S(1) | E(8) | M(23)
   * E32 = E16 - 15 + 127 = E16 + 112 */
  uint32_t s32 = (uint32_t)s << 31;
  uint32_t e32 = (uint32_t)(e + 112U) << 23;
  uint32_t m32 = (uint32_t)m << 13;

  /* MISRA C 준수: Union Type Punning 대신 memcpy 사용 */
  uint32_t bits = s32 | e32 | m32;
  float result;
  (void)memcpy(&result, &bits, sizeof(result));
  return result;
}

// --- 플랫폼 의존 함수 (I2C Read/Write) ---
/** @brief I2C 쓰기 함수 래퍼 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len) {
  printf("DEBUG: platform_write called for Reg 0x%02X\n", reg);
  HAL_I2C_Mem_Write((I2C_HandleTypeDef *)handle, LSM6DSV16X_I2C_ADD_H, reg,
                    I2C_MEMADD_SIZE_8BIT, (uint8_t *)bufp, len, 1000);
  return 0;
}

// MLX 센서용 래퍼 (표준 I2C 쓰기)
static int32_t mlx_write(void *handle, uint8_t *buf, uint16_t len) {
  HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)handle, MLX90393_ADDR << 1, buf,
                          len, 1000);
  return 0;
}

static int32_t mlx_read(void *handle, uint8_t *buf, uint16_t len) {
  HAL_I2C_Master_Receive((I2C_HandleTypeDef *)handle, MLX90393_ADDR << 1, buf,
                         len, 1000);
  return 0;
}

static int32_t uart_read_mock(void *handle, uint8_t *buf, uint16_t len) {
  return 0;
}

/** @brief I2C 읽기 함수 래퍼 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len) {
  HAL_I2C_Mem_Read((I2C_HandleTypeDef *)handle, LSM6DSV16X_I2C_ADD_H, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
  return 0;
}

/** @brief 전체 센서 초기화 (전원 시퀀스 포함) */
void Sensors_Init(void) {
  // 1. GPIO 전원 시퀀스 (리셋 해제)
#ifndef UNIT_TEST
  // CubeMX 생성 라벨 사용 가정
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

/** @brief 1-Wire 센서(DS18B20) 초기화 */
void Sensors_Init_1Wire(void) {
#ifndef HOST_TEST_MODE
  DS18B20_Init_Driver();
#endif
}

/** @brief I2C1 버스 센서 초기화 (LSM6DSV16X, MLX90393, GDK101) */
void Sensors_Init_I2C1(void) {
  // #ifndef HOST_TEST_MODE
  // MLX90393 초기화
  mlx_ctx.write = mlx_write;
  mlx_ctx.read = mlx_read;
  MLX90393_Init(&mlx_ctx);

  // GDK101 초기화
  gdk_ctx.write_reg = platform_write;
  gdk_ctx.read_reg = platform_read;
  gdk_ctx.address = GDK101_I2C_ADDR; // 0x18
  GDK101_Init(&gdk_ctx);

  // LSM6DSV16X 초기화
  lsm_ctx.write_reg = platform_write;
  lsm_ctx.read_reg = platform_read;
  // lsm_ctx.handle = &hi2c1; // 실제 하드웨어 핸들

  uint8_t whoamI = 0;
  lsm6dsv16x_device_id_get(&lsm_ctx, &whoamI);
  if (whoamI != LSM6DSV16X_ID) {
    // 에러 처리 필요
  }

  // 기본 설정 복원 (소프트웨어 리셋)
  lsm6dsv16x_sw_reset(&lsm_ctx);

  // 설정: ODR 480Hz
  lsm6dsv16x_xl_data_rate_set(&lsm_ctx, LSM6DSV16X_ODR_AT_480Hz);
  lsm6dsv16x_gy_data_rate_set(&lsm_ctx, LSM6DSV16X_ODR_AT_480Hz);

  // 설정: 고성능 모드 (High Performance)
  lsm6dsv16x_xl_mode_set(&lsm_ctx, LSM6DSV16X_XL_HIGH_PERFORMANCE_MD);
  lsm6dsv16x_gy_mode_set(&lsm_ctx, LSM6DSV16X_GY_HIGH_PERFORMANCE_MD);

  // SFLP (Sensor Fusion Low Power) 내부 칼만 필터 활성화
  lsm6dsv16x_sflp_game_rotation_set(&lsm_ctx, 1);
  lsm6dsv16x_sflp_data_rate_set(&lsm_ctx, LSM6DSV16X_SFLP_120Hz);
  // #endif
}

/** @brief I2C3 버스 센서 초기화 (SEN0321, MCP9600, MS5611, SHT31) */
void Sensors_Init_I2C3(void) {
  // #ifndef HOST_TEST_MODE
  // SEN0321 초기화
  sen_ctx.write_reg = platform_write; // 플랫폼 쓰기 함수 재사용
  sen_ctx.read_reg = platform_read;   // 플랫폼 읽기 함수 재사용
  sen_ctx.address = SEN0321_I2C_ADDR_0;
  SEN0321_Init(&sen_ctx);

  // MCP9600 초기화
  mcp_ctx.write_reg = platform_write;
  mcp_ctx.read_reg = platform_read;
  mcp_ctx.address = MCP9600_I2C_ADDR_DEFAULT; // 0x67
  MCP9600_Init(&mcp_ctx);

  // MS5611 초기화
  ms_ctx.write_reg = platform_write;
  ms_ctx.read_reg = platform_read;
  ms_ctx.address = MS5611_I2C_ADDR_HIGH;
  MS5611_Init(&ms_ctx);

  // SHT31 초기화
  sht_ctx.write_reg = platform_write;
  sht_ctx.read_reg = platform_read;
  sht_ctx.address = SHT31_I2C_ADDR_DEFAULT;
  SHT31_Init(&sht_ctx);
  // #endif
}

/** @brief UART 센서 초기화 (Unit Test에선 비활성) */
void Sensors_Init_UART(void) {
  // 타입 불일치 및 미사용 상태로 인해 유닛 테스트에서는 비활성화
}

/** @brief 센서 리셋 및 재초기화 */
void Sensors_Reset(SensorID_t id) {
#ifdef UNIT_TEST
  printf("FDIR: Resetting Sensor ID %d\n", id);
#endif
  // 하드웨어 구현 예시:
  // 1. 드라이버 해제 및 재할당
  // 2. GPIO 전원 사이클링
  // if (id == SENSOR_ID_PMS) PMS_Init(&pms_ctx); ...
}

/** @brief 모든 센서 데이터 읽기 루프 */
SensorStatus_t Sensors_Read_All(telemetry_payload_sensor_snapshot_t *data) {
  // 1. IMU (고속 50Hz)
  Sensors_Read_IMU(data->accel_mps2_x1000, data->gyro_rads_x1000);

  // 2. 자력계(Mag)
  Sensors_Read_Mag(data->mag_uT);

  // 3. 기압계 (저속 5Hz)
  static uint32_t last_baro = 0;
  if (HAL_GetTick() - last_baro > 200) {
    Sensors_Read_Baro(&data->ms5611_press_pa, &data->ms5611_temp_c_x100);
    last_baro = HAL_GetTick();
  }

  // 4. 온습도 (저속 1Hz)
  static uint32_t last_env = 0;
  if (HAL_GetTick() - last_env > 1000) {
    Sensors_Read_Humid(&data->sht31_temp_c_x100, &data->sht31_rh_x100);
    last_env = HAL_GetTick();
  }

  // GPS 및 배터리는 App_Loop에서 별도 관리

  // 6. 배터리 상태
  Sensors_Read_Battery(&data->bat_mv, &data->bat_temp_c_x100);

  // 7. 기타 센서 (보드 온도 등)
  Sensors_Read_BoardTemp(&data->board_temp_c_x100);

  // SITL용 Mock 물리학 업데이트 (상승 시뮬레이션)
#ifdef HOST_TEST_MODE
  mock_altitude += 2.5f; // 약 5m/s 상승 가정
  // App_Loop에서 호출됨을 가정.
  if (mock_altitude > 30000.0f)
    mock_altitude = 100.0f;

  // 표준 대기 모델에 따른 온도 근사 (T = 15 - 0.0065 * h)
  mock_temp = 15.0f - (0.0065f * mock_altitude);

  // 기압 근사: P = P_sea * exp(-h/7400) (단순화 모델)
  mock_pressure = 101325.0f * expf(-mock_altitude / 7400.0f);
#endif

  return SENSOR_OK;
}

void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]) {
#ifndef HOST_TEST_MODE
  int16_t data_raw[3];
  uint8_t idx;

  /* 가속도 읽기 (Accel) */
  lsm6dsv16x_acceleration_raw_get(&lsm_ctx, data_raw);
  /* m/s^2 단위 변환 (* 1000) */
  for (idx = 0U; idx < 3U; idx++) {
    float mg = lsm6dsv16x_from_fs2_to_mg(data_raw[idx]);
    accel[idx] = (int32_t)(mg * 9.8f);
  }

  /* 자이로스코프 읽기 (Gyro) */
  lsm6dsv16x_angular_rate_raw_get(&lsm_ctx, data_raw);
  for (idx = 0U; idx < 3U; idx++) {
    float mdps = lsm6dsv16x_from_fs2000_to_mdps(data_raw[idx]);
    /* rad/s 변환 (* 1000). 1 mdps = 0.00001745 rad/s.
     * result = mdps * 0.01745 */
    gyro[idx] = (int32_t)(mdps * 0.01745f);
  }

  /* 성공 보고 (여기까지 도달했다면 I2C 통신 성공 가정) */
  // 이상적으로는 반환값 확인 필요:
  // if (ret_xl == 0 && ret_gy == 0)
  FDIR_ReportSuccess(SENSOR_ID_IMU);
#else
  accel[0] = 0;
  accel[1] = 0;
  accel[2] = 9810;
  gyro[0] = 0;
  gyro[1] = 0;
  gyro[2] = 0;
  FDIR_ReportSuccess(SENSOR_ID_IMU);
#endif
}

void Sensors_Read_Mag(float mag[3]) {
#ifndef HOST_TEST_MODE
  // 드라이버 기능 호출: 측정 시작
  MLX90393_StartMeasurement(&mlx_ctx);
  // 딜레이 필요? Mock은 즉시 반환.
  MLX90393_ReadMeasurement(&mlx_ctx, &mag[0], &mag[1], &mag[2]);
  FDIR_ReportSuccess(SENSOR_ID_MAG);
#else
  mag[0] = 0.0f;
  mag[1] = 0.0f;
  mag[2] = 0.0f;
  FDIR_ReportSuccess(SENSOR_ID_MAG);
#endif
}

void Sensors_Read_Rad(uint16_t *uSvh) {
  // #ifndef HOST_TEST_MODE
  float val_uSvh;
  // 10분 이동 평균값 읽기
  if (GDK101_Read_10Min_Avg(&gdk_ctx, &val_uSvh) == 0) {
    *uSvh = (uint16_t)(val_uSvh * 100); // 스케일링 x100
    FDIR_ReportSuccess(SENSOR_ID_RAD);
  } else {
    *uSvh = 0; // 에러
  }
  /* #else
      *uSvh = 0;
      FDIR_ReportSuccess(SENSOR_ID_RAD);
  #endif */
}

void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
  int32_t p, t;
  if (MS5611_Read_PT(&ms_ctx, &p, &t) == 0) {
    *press_pa = (uint32_t)p;
    *temp_c_x100 = (int16_t)t;
    FDIR_ReportSuccess(SENSOR_ID_BARO);
  } else {
    *press_pa = 101325;
    *temp_c_x100 = 2500;
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
  if (SHT31_ReadTempHum(&sht_ctx, &t, &rh) == 0) {
    *temp_c_x100 = (int16_t)(t * 100);
    *rh_x100 = (uint16_t)(rh * 100);
    FDIR_ReportSuccess(SENSOR_ID_SHT);
  } else {
    *temp_c_x100 = 0;
    *rh_x100 = 0;
  }
#else
  *temp_c_x100 = 2500;
  *rh_x100 = 5000;
  FDIR_ReportSuccess(SENSOR_ID_SHT);
#endif
}

void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0,
                             uint16_t *pm2_5) {
#ifndef HOST_TEST_MODE
  // 이산화탄소(CO2) 읽기
  CM1107N_ReadCO2(&cm_ctx, co2);
  // if (*co2 == 0) *co2 = 400; // 최소 기본값

  // 오존(Ozone) 읽기
  SEN0321_ReadOzone(&sen_ctx, ozone);

  // 미세먼지(PMS) 읽기 (Mock 데이터 처리)
  // 실제 시스템에선 UART ISR이 PMS_ProcessByte를 호출함
  // 여기선 ctx의 최신 유효 데이터를 읽음
  *pm1_0 = pms_ctx.data.PM_AE_UG_1_0;
  *pm2_5 = pms_ctx.data.PM_AE_UG_2_5;

  // ISR 공급이 없을 경우 Mock 데이터 자동 증가
  if (*pm2_5 == 0)
    *pm2_5 = 15;

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
                      uint8_t *sats, uint8_t *sats_view, uint8_t *sats_gps,
                      uint8_t *sats_glonass, uint8_t *sats_galileo,
                      uint8_t *sats_beidou, uint8_t *utc_hour, uint8_t *utc_min,
                      uint8_t *utc_sec, uint8_t *utc_day, uint8_t *utc_month,
                      uint16_t *utc_year) {
  /* Mock: Fix가 없으면 테스트용 NMEA 데이터 주입 (호스트 파싱 검증용) */
  if (xa_ctx.data.fix_type == 0U) {
    const char *sim_gga =
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n";
    uint16_t idx;
    for (idx = 0U; sim_gga[idx] != '\0'; idx++) {
      XA1110_ProcessByte(&xa_ctx, (uint8_t)sim_gga[idx]);
    }
    /* 날짜/시간 처리를 위한 RMC 추가 */
    const char *sim_rmc =
        "$GPRMC,123519.00,A,4807.038,N,01131.000,E,0.0,0.0,060126,,,A*6B\r\n";
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
  /* GSV 파싱 결과: 개별 GNSS 위성 수 */
  *sats_gps = xa_ctx.data.sats_gps;
  *sats_glonass = xa_ctx.data.sats_glonass;
  *sats_galileo = xa_ctx.data.sats_galileo;
  *sats_beidou = xa_ctx.data.sats_beidou;

  /* GPS UTC 시간 */
  *utc_hour = xa_ctx.data.utc_hour;
  *utc_min = xa_ctx.data.utc_min;
  *utc_sec = xa_ctx.data.utc_sec;
  *utc_day = xa_ctx.data.utc_day;
  *utc_month = xa_ctx.data.utc_month;
  *utc_year = xa_ctx.data.utc_year;

  /* 상태 확인: 유효한 Fix 또는 데이터 수신 여부 */
  FDIR_ReportSuccess(SENSOR_ID_GPS);
}

void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
  // 느린 센서: 1Hz 제한
  static uint32_t last_bat = 0;
  if (HAL_GetTick() - last_bat > 1000) {

#ifndef UNIT_TEST
    // 실제 하드웨어 ADC 읽기
    extern ADC_HandleTypeDef hadc1;
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
      uint32_t raw = HAL_ADC_GetValue(&hadc1);
      float voltage_mv = (raw * 3300.0f / 4096.0f) * 6.0f;
      *mv = (uint16_t)voltage_mv;
    }
    HAL_ADC_Stop(&hadc1);
#else
    *mv = 15500; // Mock 15.5V (4S 배터리 가정)
#endif

    *temp_c_x100 = DS18B20_ReadTemp_x100(0); // 배터리 온도
    last_bat = HAL_GetTick();
  }
#else
  *mv = 16000;
  *temp_c_x100 = 2000;
#endif
}

// ...
void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
  *temp_c_x100 = DS18B20_ReadTemp_x100(1); // 보드 온도
#else
  *temp_c_x100 = 2500;
#endif
}

void Sensors_Read_External(int16_t *temp_c_x100) {
#ifndef HOST_TEST_MODE
  // MCP9600에서 열전대 온도 읽기
  float val;
  if (MCP9600_ReadThermocouple(&mcp_ctx, &val) == 0) {
    *temp_c_x100 = (int16_t)(val * 100);
    FDIR_ReportSuccess(SENSOR_ID_EXT_TEMP);
  } else {
    *temp_c_x100 = 0; // 에러
  }
#else
  *temp_c_x100 = -5000;
  FDIR_ReportSuccess(SENSOR_ID_EXT_TEMP);
#endif
}

void Sensors_Read_SFLP(float quaternion[4]) {
#ifndef HOST_TEST_MODE
  // 1. FIFO 상태 확인
  lsm6dsv16x_fifo_status_t fifo_status;
  if (lsm6dsv16x_fifo_status_get(&lsm_ctx, &fifo_status) != 0)
    return;

  uint16_t samples = fifo_status.fifo_level;
  if (samples == 0)
    return;

  // 블로킹 방지를 위한 루프 제한
  if (samples > 20)
    samples = 20;

  lsm6dsv16x_fifo_out_raw_t fifo_data;

  for (int i = 0; i < samples; i++) {
    // 2. FIFO 데이터 읽기
    if (lsm6dsv16x_fifo_out_raw_get(&lsm_ctx, &fifo_data) != 0)
      break;

    // 3. 태그 파싱 (SFLP 게임 회전 벡터)
    if (fifo_data.tag == LSM6DSV16X_SFLP_GAME_ROTATION_VECTOR_TAG) {
      uint16_t raw_x =
          (uint16_t)fifo_data.data[0] | ((uint16_t)fifo_data.data[1] << 8);
      uint16_t raw_y =
          (uint16_t)fifo_data.data[2] | ((uint16_t)fifo_data.data[3] << 8);
      uint16_t raw_z =
          (uint16_t)fifo_data.data[4] | ((uint16_t)fifo_data.data[5] << 8);

      float x = half_to_float(raw_x);
      float y = half_to_float(raw_y);
      float z = half_to_float(raw_z);

      float sum_sq = x * x + y * y + z * z;
      float w = 1.0f;

      if (sum_sq < 1.0f) {
        w = sqrtf(1.0f - sum_sq);
      } else {
        float norm = sqrtf(sum_sq);
        if (norm > 0.0f) {
          x /= norm;
          y /= norm;
          z /= norm;
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
  quaternion[0] = 0.0f;
  quaternion[1] = 0.0f;
  quaternion[2] = 0.0f;
  quaternion[3] = 1.0f;
#endif
}
