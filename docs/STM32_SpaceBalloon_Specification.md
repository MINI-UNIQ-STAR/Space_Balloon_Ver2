# STM32 성층권 풍선 센서 플랫폼 사양서

**버전:** Rev 3.5 (코드-문서 일관성 종합 검토)
**날짜:** 2026-01-08
**MCU:** STM32G431CBU6 (Cortex-M4F @ 170MHz)
**구조:** Bare-metal (Super Loop + HAL)

---

## 목차

1. [시스템 개요](#시스템-개요)
2. [하드웨어 사양](#하드웨어-사양)
3. [센서 구성](#센서-구성)
4. [통신 프로토콜](#통신-프로토콜)
5. [전원 관리](#전원-관리)
6. [헬스 모니터링](#헬스-모니터링)
7. [히터 제어](#히터-제어)
8. [텔레메트리 시스템](#텔레메트리-시스템)
9. [핀 매핑](#핀-매핑)
10. [빌드 환경](#빌드-환경)

---

## 시스템 개요

### 주요 기능
- **11개 센서** 동시 운용 (IMU, 자기계, 방사선, 온도, GPS, 오존, 습도, 기압, 열전대, CO2, 미세먼지)
- **50Hz 텔레메트리** 전송 (GPS 1PPS 동기화)
- **자동 복구 시스템** (I2C/UART/1-Wire 버스 복구)
- **PID 히터 제어** (배터리/보드 온도 유지)
- **칼만 필터** 기반 고도 융합
- **건강 모니터링** (2000ms 타임아웃 감시)

### 아키텍처
```
┌─────────────────────────────────────────────────────┐
│            STM32G431CBU6 (170MHz)                   │
│  ┌──────────────────────────────────────────────┐   │
│  │         Bare-metal (Super Loop)              │   │
│  ├──────────────────────────────────────────────┤   │
│  │  Application Layer (app.c)                   │   │
│  │  - Sensors_Read_All()                        │   │
│  │  - PID Heater Control                        │   │
│  │  - Kalman Filter (Altitude)                  │   │
│  │  - FDIR (Fault Detection)                    │   │
│  │  - Telemetry (50Hz)                          │   │
│  │  - XCP Calibration Protocol                  │   │
│  ├──────────────────────────────────────────────┤   │
│  │  BSP/Drivers Layer (bsp.c, sensors.c)        │   │
│  │  - I2C/UART/1-Wire Drivers                   │   │
│  │  - Sensor Codecs, Reset Lines Control        │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
         │            │           │
      I2C1/3       UART1/2/3   ADC/GPIO
         │            │           │
    ┌────┴────┐  ┌───┴───┐  ┌───┴────┐
    │Sensors  │  │ GPS   │  │Heaters │
    │(11ea)   │  │PMS3003│  │Battery │
    └─────────┘  └───────┘  └────────┘
```

---

## 하드웨어 사양

### MCU 특성
| 항목 | 사양 |
|------|------|
| 코어 | ARM Cortex-M4F (FPU 포함) |
| 클럭 | 170MHz (최대) |
| 플래시 | 112KB (사용: 88,552 bytes / 77.2%) |
| SRAM | 32KB (사용: 14,264 bytes / 43.5%) |
| FPU | fpv4-sp-d16 (hard float ABI) |

### 주변장치 사용
- **I2C1**: Downside 센서 보드 (LSM6DSV16X, MLX90393, GDK101)
- **I2C3**: Upside 센서 보드 (CM1107N, MCP9600, SEN0321, SHT31-D, MS5611)
- **UART1**: XA1110 GPS 모듈 (115200 baud)
- **UART2**: PMS3003 미세먼지 센서 (9600 baud)
- **UART3**: LoRa32 텔레메트리 출력 (115200 baud)
- **ADC1_IN2 (PA1)**: 배터리 전압 측정
- **TIM3_CH1 (PA6)**: 배터리 히터 PWM (Kapton 필름)
- **TIM8_CH1 (PC6)**: 보드 히터 PWM (Minibulb)
- **1-Wire (PB15)**: DS18B20 온도 센서 x2 (배터리 + 보드)

---

## 센서 구성

### 전체 센서 목록 (11종)

| # | 센서 모델 | 측정 항목 | 인터페이스 | 주소/설정 | 구현 파일 |
|---|-----------|-----------|------------|-----------|-------------|
| 1 | LSM6DSV16X | 6축 IMU (가속도/자이로) | I2C1 | 0x6B | sensors.c |
| 2 | MLX90393 | 3축 자기계 | I2C1 | 0x0C | sensors.c |
| 3 | GDK101 | 방사선 (γ선) | I2C1 | 0x18 | sensors.c |
| 4 | DS18B20 x2 | 온도 (배터리/보드) | 1-Wire (PB15) | - | sensors.c |
| 5 | XA1110 | GPS | UART1 (9600) | NMEA | sensors.c |
| 6 | SEN0321 | 오존 (O3) | I2C3 | 0x70 | sensors.c |
| 7 | SHT31-D | 온습도 | I2C3 | 0x44 | sensors.c |
| 8 | MS5611 | 기압/온도 | I2C3 | 0x77 | sensors.c |
| 9 | MCP9600 | 열전대 (K-type) | I2C3 | 0x60 | sensors.c |
| 10 | CM1107N | CO2 농도 | I2C3 | 0x31 | sensors.c |
| 11 | PMS3003 | 미세먼지 (PM1.0/2.5/10) | UART2 (9600) | - | sensors.c |

### 센서별 상세 사양

#### 1. LSM6DSV16X (IMU)
- **측정 범위**: ±16g (가속도), ±2000dps (자이로)
- **출력 포맷**: 가속도 (m/s² x1000), 자이로 (rad/s x1000)
- **샘플링**: 서비스 틱마다 업데이트
- **인터럽트**: PB6 (LSM_INT) - EXTI6

#### 2. MLX90393 (자기계)
- **측정 범위**: 3축 자기장 (µT)
- **출력 포맷**: float (µT)
- **인터럽트**: PB7 (MLX_INT) - EXTI7

#### 3. GDK101 (방사선 검출기)
- **측정 항목**: γ선 선량율
- **출력 포맷**: µSv/h x100 (uint16_t)
- **측정 주기**: 1분 평균

#### 4. DS18B20 (온도 센서 x2)
- **정밀도**: 12-bit (0.0625°C)
- **출력 포맷**: °C x100 (int16_t)
- **변환 시간**: 750ms
- **할당**: 인덱스 0 = 배터리, 인덱스 1 = 보드

#### 5. XA1110 (GPS)
- **프로토콜**: NMEA 0183
- **보레이트**: 9600 baud
- **지원 메시지**: GGA, GSA, GSV, RMC, VTG
- **GNSS**: GPS, GLONASS, Galileo, BeiDou
- **1PPS 출력**: PB4 (GPS_PPS) - EXTI4
- **제어 핀**:
  - PA7 (GPS_Wake): 웨이크업 제어
  - PA9 (GPS_nRST): 리셋 제어 (active-low)
  - PB12 (GPS_INT): 인터럽트 입력

#### 6. SEN0321 (오존 센서)
- **측정 범위**: 0-500 ppb
- **출력 포맷**: int16_t (ppb)

#### 7. SHT31-D (온습도)
- **측정 항목**: 온도, 상대습도
- **출력 포맷**:
  - 온도: °C x100 (int16_t)
  - 습도: %RH x100 (uint16_t)
- **리셋 제어**: PB11 (SHT_RST) - P-MOS (HIGH=OFF, LOW=ON)

#### 8. MS5611 (기압계)
- **측정 범위**: 10-1200 mbar
- **출력 포맷**:
  - 기압: Pa (uint32_t)
  - 온도: °C x100 (int16_t)
- **고도 계산**: 표준 대기 공식 사용

#### 9. MCP9600 (열전대)
- **센서 타입**: K-type 열전대
- **측정 항목**: 실내 2차 온도
- **출력 포맷**: °C x100 (int16_t)

#### 10. CM1107N (CO2 센서)
- **측정 범위**: 400-5000 ppm
- **출력 포맷**: ppm (uint16_t)

#### 11. PMS3003 (미세먼지)
- **측정 항목**: PM1.0, PM2.5, PM10
- **출력 포맷**: µg/m³ (uint16_t)
- **프로토콜**: 시리얼 (9600 baud)
- **제어**: PB10 (PMS_SET)

---

## 통신 프로토콜

### I2C 버스 구성

#### I2C1 (Downside Board)
- **핀**: PB9 (SDA), PA15 (SCL)
- **속도**: 400kHz (Fast Mode)
- **센서**: LSM6DSV16X, MLX90393, GDK101
- **복구 메커니즘**:
  - SCL 클럭 펄스 생성 (9회)
  - 소프트웨어 리셋 시퀀스
  - 구현: `Core/Src/i2c.c`

#### I2C3 (Upside Board)
- **핀**: PB5 (SDA), PA8 (SCL)
- **속도**: 400kHz (Fast Mode)
- **센서**: CM1107N, MCP9600, SEN0321, SHT31-D, MS5611
- **복구 메커니즘**: I2C1과 동일

### UART 구성

#### UART1 (GPS)
- **핀**: PC4 (TX), PA10 (RX)
- **속도**: 115200 baud, 8N1
- **DMA**: RX DMA 사용 (DMA1_Channel1)
- **파싱**: `Core/Src/sensors.c` (XA1110 드라이버)

#### UART2 (PMS3003)
- **핀**: PA2 (TX), PA3 (RX)
- **속도**: 9600 baud, 8N1
- **DMA**: RX DMA 사용 (DMA1_Channel3)
- **파싱**: `Core/Src/sensors.c` (PMS3003 드라이버)

#### UART3 (텔레메트리)
- **핀**: PC10 (TX), PC11 (RX)
- **속도**: 115200 baud, 8N1 (LoRa32 모듈 연결)
- **용도**: 텔레메트리 데이터 전송

### 1-Wire (DS18B20)
- **핀**: PB15
- **모드**: 비트뱅 (GPIO 재구성)
  - LOW 출력: Open-drain
  - 읽기: Input pull-up
- **타이밍**: 표준 1-Wire 프로토콜
- **구현**: `Core/Src/sensors.c` (DS18B20 드라이버)

---

## 전원 관리

### 센서 전원 제어 (P-MOS 기반)

각 센서/모듈마다 개별 P-MOS 스위치(AO3401F)를 통한 전원 제어:

| 센서 | 리셋 라인 | GPIO 핀 | 초기 상태 | 제어 |
|------|-----------|---------|-----------|------|
| LSM6DSV16X | LSM_RST | PB13 | LOW | bsp.c |
| MLX90393 | MLX_RST | PB14 | HIGH | bsp.c |
| CM1107N | CM1107N_RST | PB0 | LOW | bsp.c |
| MS5611 | MS_RST | PA5 | LOW | bsp.c |
| MCP9600 | MCP_RST | PA4 | LOW | bsp.c |
| SHT31-D | SHT_RST | PB11 | - | bsp.c |
| GDK101 | GDK_RST | PB2 | - | bsp.c |
| PMS3003 | PMS_SET | PB10 | HIGH | bsp.c |
| Sensor Board | SEN_RST | PB1 | - | bsp.c |

### 전원 제어 API
```c
// Core/Inc/bsp.h
void BSP_Init(void);
void BSP_Sensor_PowerOn(void);  // 리셋 라인 해제 (GPIO SET)
```

### 배터리 모니터링
- **ADC 채널**: ADC1_IN2 (PA1)
- **분압비**: 6:1 (raw * 3300 / 4096 * 6)
- **출력**: mV (uint16_t)
- **구현**: `Core/Src/bsp.c` (BSP_ADC_Read_Battery_mV)

---

## 헬스 모니터링

### 감시 메커니즘
- **체크 주기**: 100ms
- **타임아웃 임계값**:
  - 일반 센서: 2000ms
  - PMS3003: 3000ms (느린 응답 고려)
- **복구 시도**: 최대 5회
- **구현**: `Core/Src/fdir.c`

### 감시 대상
```c
// Core/Inc/fdir.h
typedef struct {
    uint32_t last_valid_update_ms;
    uint32_t error_count;
    uint32_t recovery_count;
    FdirState_t state;
    bool enabled;
} SensorHealth_t;

typedef struct {
    SensorHealth_t imu;
    SensorHealth_t baro;
    SensorHealth_t gps;
    SensorHealth_t co2;
    SensorHealth_t pms;
    SensorHealth_t sht;
    SensorHealth_t rad;
    SensorHealth_t ext_temp;
} SystemHealth_t;
```

각 센서별 상태 추적:
- GPS (UART1)
- PMS3003 (UART2)
- I2C1 센서들 (LSM, MLX, GDK)
- I2C3 센서들 (SHT, MS, CO2, MCP, SEN0321)
- DS18B20 온도 센서

### 자동 복구 절차
1. 타임아웃 감지 (2000ms 동안 업데이트 없음)
2. 복구 시도 카운터 증가
3. **I2C 복구**:
   - SCL 라인에 9개 클럭 펄스 생성
   - I2C 초기화 재시도
4. **센서 하드웨어 리셋**:
   - P-MOS 제어를 통한 전원 사이클
   - 리셋 라인 펄스 생성
5. 최대 시도 횟수(5회) 초과 시 영구 실패 마킹

---

## 히터 제어

### 히터 시스템 구성

#### 배터리 히터 (Kapton Film)
- **PWM 채널**: TIM3_CH1 (PA6)
- **온도 센서**: DS18B20 인덱스 0
- **목표 온도**: 10°C (조정 가능)
- **PID 상수**:
  - Kp: 1000.0
  - Ki: 10.0
  - Kd: 0.0
  - Integral Max: 30000.0

#### 보드 히터 (Minibulb)
- **PWM 채널**: TIM8_CH1 (PC6)
- **온도 센서**: DS18B20 인덱스 1
- **목표 온도**: 5°C (조정 가능)
- **PID 상수**:
  - Kp: 500.0
  - Ki: 5.0
  - Kd: 0.0
  - Integral Max: 15000.0

### PID 제어 알고리즘
```c
// Core/Src/pid.c
float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt) {
    float error = hpid->Target - measurement;
    hpid->IntegratedError += error * dt;  // Anti-windup by MaxOutput
    float p_term = hpid->Kp * error;
    float i_term = hpid->Ki * hpid->IntegratedError;
    float d_term = hpid->Kd * (error - hpid->LastError) / dt;
    float output = p_term + i_term + d_term;
    hpid->LastError = error;
    return (output > hpid->MaxOutput) ? hpid->MaxOutput : 
           (output < 0) ? 0 : output;
}
```

### 제어 주기
- **업데이트 주기**: 50Hz (20ms, App_Loop 내에서 호출)
- **안전 장치**: 온도 센서 실패 시 히터 자동 OFF

### API
```c
// Core/Inc/pid.h
void PID_Init(PID_HandleTypeDef *hpid, float Kp, float Ki, float Kd, float MaxOutput);
float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt);

// Core/Inc/actuators.h
void Actuators_SetHeater_Battery(float duty_percent);
void Actuators_SetHeater_Board(float duty_percent);
```

---

## 텔레메트리 시스템

### 전송 사양
- **전송 속도**: 50Hz (20ms 간격)
- **동기화**: GPS 1PPS 신호 기준
- **프로토콜**: 바이너리 프레이밍
- **페이로드 크기**: 116 바이트 (packed struct)
- **전체 프레임**: 132 바이트 (헤더 14 + 페이로드 116 + CRC 2)
- **CRC**: CRC16-CCITT-FALSE 체크섬
- **출력**: UART3 → LoRa32 모듈
- **RF 설정**: 915 MHz, SF11, BW 125kHz, CR 4/5 (장거리 전송 최적화)

### 프레임 구조
```c
// Core/Inc/telemetry.h
typedef struct __attribute__((packed)) {
    uint8_t magic[2];           // 0xA5, 0x5A
    uint8_t version;            // 프로토콜 버전 (1)
    uint8_t msg_type;           // 메시지 타입 (0x01=heartbeat, 0x02=sensor snapshot)
    uint16_t payload_len;       // 페이로드 길이 (바이트)
    uint16_t seq;               // 시퀀스 번호
    uint32_t timestamp_ms;      // 시스템 타임스탬프 (ms)
    telemetry_payload_sensor_snapshot_t payload;
    uint16_t crc16;             // CRC16 체크섬
} telemetry_frame_t;
```

### 센서 스냅샷 페이로드
```c
typedef struct __attribute__((packed)) {
    // 1. 시스템 상태
    uint32_t uptime_ms;               // 시스템 가동 시간 (ms)
    uint16_t status_flags;            // FDIR 상태 플래그
    uint16_t co2_ppm;                 // CO2 농도 (ppm)

    // 2. IMU (가속도, 자이로)
    int32_t accel_mps2_x1000[3];      // x, y, z (m/s² x1000)
    int32_t gyro_rads_x1000[3];       // x, y, z (rad/s x1000)

    // 3. 자기계
    float mag_uT[3];                  // x, y, z (µT)

    // 4. 온도 측정
    int16_t board_temp_c_x100;        // 보드 온도 - DS18B20 (°C x100)
    int16_t external_temp_c_x100;     // 외부 온도 - MCP9600 (°C x100)
    int16_t sht31_temp_c_x100;        // SHT31 온도 (°C x100)
    int16_t bat_temp_c_x100;          // 배터리 온도 - DS18B20 (°C x100)

    // 5. GPS 데이터
    int32_t gps_lat_deg_e7;           // 위도 (도 x 10^7)
    int32_t gps_lon_deg_e7;           // 경도 (도 x 10^7)
    float gps_alt_m;                  // GPS 고도 (m)
    uint8_t gps_fix;                  // Fix 상태 (0=No, 1=2D, 2=3D)
    uint8_t gps_sats_used;            // 사용 중인 위성 수
    uint8_t gps_sats_in_view_total;   // 총 가시 위성
    uint8_t gps_sats_in_view_gps;     // GPS 위성
    uint8_t gps_sats_in_view_glonass; // GLONASS 위성
    uint8_t gps_sats_in_view_galileo; // Galileo 위성
    uint8_t gps_sats_in_view_beidou;  // BeiDou 위성

    // 6. GPS UTC 시간 (RMC 문장에서 파싱)
    uint8_t gps_utc_hour;             // 시 (0-23)
    uint8_t gps_utc_min;              // 분 (0-59)
    uint8_t gps_utc_sec;              // 초 (0-59)
    uint8_t gps_utc_day;              // 일 (1-31)
    uint8_t gps_utc_month;            // 월 (1-12)
    uint16_t gps_utc_year;            // 년 (2000-2099)

    // 7. 배터리
    uint16_t bat_mv;                  // 배터리 전압 (mV)

    // 8. 대기질
    uint16_t pm1_ugm3;                // PM1.0 (µg/m³)
    uint16_t pm25_ugm3;               // PM2.5 (µg/m³)
    uint16_t pm10_ugm3;               // PM10 (µg/m³)
    int16_t ozone_ppb;                // 오존 (ppb)

    // 9. 기압/습도
    uint16_t sht31_rh_x100;           // 습도 (%RH x100)
    uint32_t ms5611_press_pa;         // 기압 (Pa)
    int16_t ms5611_temp_c_x100;       // MS5611 온도 (°C x100)

    // 10. 방사선
    uint16_t gdk101_usvh_x100;        // 선량율 (µSv/h x100)

    // 11. 히터 상태
    uint8_t heater_bat_duty_percent;  // 배터리 히터 듀티 (%)
    uint8_t heater_board_duty_percent;// 보드 히터 듀티 (%)

    // 12. 고도 추정 (Kalman Filter)
    float press_alt_m;                // 기압 고도 (m)
    float kf_alt_m;                   // 칼만 필터 고도 (m)
    float kf_roll_deg;                // Roll (도)
    float kf_pitch_deg;               // Pitch (도)
} telemetry_payload_sensor_snapshot_t;
```

### GPS 1PPS 동기화
- **1PPS 입력**: PB4 (GPS_PPS)
- **동작**:
  1. 1PPS 에지 검출 (EXTI4 인터럽트)
  2. 1초 구간을 50개 슬롯으로 분할 (각 20ms)
  3. 각 슬롯마다 텔레메트리 프레임 전송
  4. 1PPS 신호 손실 시 free-running 모드로 전환

### 구현
- **텔레메트리 전송**: `Core/Src/telemetry.c`
- **프레임 정의**: `Core/Inc/telemetry.h`
- **1PPS 캡처**: `drivers/pps_capture.c`

---

## 핀 매핑

### GPIO 핀 할당표

| MCU 핀 | 신호 이름 | 모드 | 기능 | 연결 대상 |
|--------|-----------|------|------|-----------|
| **포트 A** |
| PA0 | - | GPIO Input | (미사용) | - |
| PA1 | ADC1_IN2 | Analog | 배터리 전압 측정 | BAT_measure |
| PA2 | USART2_TX | AF PP | PMS3003 TX | PMS3003 RX |
| PA3 | USART2_RX | AF PP | PMS3003 RX | PMS3003 TX |
| PA4 | MCP_RST | GPIO Output | MCP9600 리셋 | MCP9600 RST |
| PA5 | MS_RST | GPIO Output | MS5611 리셋 | MS5611 RST |
| PA6 | TIM3_CH1 | AF PP | 배터리 히터 PWM | Kapton Film |
| PA7 | GPS_Wake | GPIO Output | GPS 웨이크업 | XA1110 Wake |
| PA8 | I2C3_SCL | AF OD | I2C3 클럭 | CM1107N, MCP9600 |
| PA9 | GPS_nRST | GPIO Output | GPS 리셋 | XA1110 nRST |
| PA10 | USART1_RX | AF PP | GPS RX | XA1110 TX |
| PA15 | I2C1_SCL | AF OD | I2C1 클럭 | Downside 센서 |
| **포트 B** |
| PB0 | CM1107N_RST | GPIO Output | CO2 센서 리셋 | CM1107N RST |
| PB1 | SEN_RST | GPIO Output | 센서 보드 리셋 | Sensor Board |
| PB2 | GDK_RST | GPIO Output | 방사선 센서 리셋 | GDK101 RST |
| PB9 | I2C1_SDA | AF OD | I2C1 데이터 | Downside 센서 |
| PC6 | TIM8_CH1 | AF PP | 보드 히터 PWM | Minibulb |
| PB4 | GPS_PPS | EXTI4 (Rising) | GPS 1PPS 입력 | XA1110 1PPS |
| PB5 | I2C3_SDA | AF OD | I2C3 데이터 | CM1107N, MCP9600 |
| PB6 | LSM_INT | EXTI6 (Rising) | IMU 인터럽트 | LSM6DSV16X INT |
| PB7 | MLX_INT | EXTI7 (Rising) | 자기계 인터럽트 | MLX90393 INT |
| PB10 | PMS_SET | GPIO Output | PMS3003 제어 | PMS3003 SET |
| PB11 | SHT_RST | GPIO Output | SHT31 리셋 | SHT31 RST |
| PB12 | GPS_INT | EXTI12 (Rising) | GPS 인터럽트 | XA1110 INT |
| PB13 | LSM_RST | GPIO Output | IMU 리셋 | LSM6DSV16X RST |
| PB14 | MLX_nRST | GPIO Output | 자기계 리셋 | MLX90393 nRST |
| PB15 | DS18B20_DQ | GPIO (재구성) | 1-Wire 데이터 | DS18B20 x2 |
| **포트 C** |
| PC4 | USART1_TX | AF PP | GPS TX | XA1110 RX |
| PC6 | TIM8_CH1 | AF PP | (타이머 채널) | - |
| PC10 | USART3_TX | AF PP | 텔레메트리 TX | LoRa32 RX |
| PC11 | USART3_RX | AF PP | 텔레메트리 RX | LoRa32 TX |
| PC13 | RTC_OUT1 | AF PP | RTC 1Hz 출력 | - |

### 인터럽트 벡터

| IRQ Handler | 핀 | 용도 |
|-------------|-----|------|
| EXTI4_IRQHandler | PB4 | GPS 1PPS (Rising) |
| EXTI9_5_IRQHandler | PB6, PB7 | LSM_INT, MLX_INT (Rising) |
| EXTI15_10_IRQHandler | PB12 | GPS_INT (Rising) |
| I2C1_EV_IRQHandler | - | I2C1 이벤트 |
| I2C1_ER_IRQHandler | - | I2C1 에러 |
| I2C3_EV_IRQHandler | - | I2C3 이벤트 |
| I2C3_ER_IRQHandler | - | I2C3 에러 |
| USART1_IRQHandler | - | GPS UART |
| USART2_IRQHandler | - | PMS3003 UART |
| USART3_IRQHandler | - | 텔레메트리 UART |
| DMA1_Channel1_IRQHandler | - | USART1 RX DMA |
| DMA1_Channel2_IRQHandler | - | ADC1 DMA |
| DMA1_Channel3_IRQHandler | - | USART2 RX DMA |
| TIM3_IRQHandler | - | 타이머3 |
| PVD_PVM_IRQHandler | - | 전압 감시 |

---

## 빌드 환경

### 빌드 시스템
- **플랫폼**: PlatformIO
- **프레임워크**: STM32Cube HAL
- **컴파일러**: arm-none-eabi-gcc
- **링커 스크립트**: STM32G431CBUx_FLASH.ld

### 디렉토리 구조
```
spaceballoon_stm32_lora32/
├── Core/
│   ├── Inc/                         # 모든 헤더 파일 (플랫 구조)
│   │   ├── telemetry.h              # 텔레메트리 프레임/페이로드 정의
│   │   ├── sensors.h                # 센서 API
│   │   ├── fdir.h                   # FDIR 모듈
│   │   ├── kalman.h                 # 칼만 필터
│   │   ├── app.h                    # 애플리케이션 진입점
│   │   ├── bsp.h                    # 보드 지원 패키지
│   │   ├── main.h                   # HAL 초기화
│   │   └── stm32g4xx_hal_conf.h     # HAL 설정
│   └── Src/                         # 모든 소스 파일 (플랫 구조)
│       ├── app.c                    # 애플리케이션 진입점
│       ├── sensors.c                # 센서 드라이버
│       ├── telemetry.c              # 텔레메트리 전송
│       ├── fdir.c                   # FDIR 구현
│       ├── kalman.c                 # 칼만 필터
│       ├── main.c                   # HAL 초기화
│       └── stm32g4xx_it.c           # 인터럽트 핸들러
├── Drivers/
│   ├── CMSIS/
│   └── STM32G4xx_HAL_Driver/
├── HostSim/                         # SITL 시뮬레이션 환경
├── test/                            # 단위/통합 테스트
├── docs/                            # 문서
├── reference/                       # 참조 자료
│   └── STM32G431/
├── CMakeLists.txt                   # CMake 빌드 설정
├── STM32G431CBUx_FLASH.ld          # 링커 스크립트
└── README.md                        # 프로젝트 개요
```

---

## 소프트웨어 아키텍처

### 초기화 순서
```c
// Core/Src/app.c - App_Init()
void App_Init(void) {
    BSP_Init();                   // 보드 지원 패키지 (GPIO, UART, I2C)
    Sensors_Init();               // 모든 센서 초기화 (I2C1, I2C3, UART, 1-Wire)
    
    // PID 히터 제어
    PID_Init(&hpid_bat, 1000.0f, 10.0f, 0.0f, 100.0f);  // 배터리
    hpid_bat.Target = 10.0f;      // 목표 온도 10°C
    PID_Init(&hpid_brd, 500.0f, 5.0f, 0.0f, 100.0f);    // 보드
    hpid_brd.Target = 5.0f;       // 목표 온도 5°C
    
    KF_Init(&hkf, 0.02f, 0.5f, 0.3f);  // 칼만 필터 (50Hz)
    XCP_Init();                   // XCP 캘리브레이션 프로토콜
    Actuators_Init();             // 히터 PWM
    FDIR_Init();                  // 고장 감지/복구
    
    // 텔레메트리 헤더 초기화
    telem_frame.magic[0] = 0xA5;
    telem_frame.magic[1] = 0x5A;
    telem_frame.version = 1;
    telem_frame.msg_type = 0x02;  // Sensor Snapshot
}
```

### 메인 루프 (20ms 주기)
```c
// Core/Src/app.c - App_Loop()
void App_Loop(void) {
    // 1. 센서 데이터 수집
    Sensors_Read_All(&telem_frame.payload);
    Sensors_Read_GPS(...);
    Sensors_Read_Battery(...);
    Sensors_Read_BoardTemp(...);
    
    // 2. FDIR 처리
    FDIR_UpdateTemperature(ext_temp);
    FDIR_UpdateGPSAltitude(gps_alt);
    FDIR_UpdateBaroAltitude(baro_alt);
    
    // 3. 자세 추정 (Quaternion → Euler)
    Sensors_Read_SFLP(quat);
    telem_frame.payload.kf_roll_deg = roll;
    telem_frame.payload.kf_pitch_deg = pitch;
    
    // 4. PID 히터 제어
    heater_battery_cmd = PID_Update(&hpid_bat, bat_temp, 0.02f);
    heater_board_cmd = PID_Update(&hpid_brd, brd_temp, 0.02f);
    Actuators_SetHeater_Battery(heater_battery_cmd);
    Actuators_SetHeater_Board(heater_board_cmd);
    
    // 5. 칼만 필터 업데이트
    KF_Predict(&hkf);
    KF_Update_Altitude(&hkf, baro_alt);
    telem_frame.payload.kf_alt_m = hkf.x[0];
    
    // 6. 상태 플래그 및 텔레메트리 전송
    telem_frame.payload.status_flags = FDIR_GetStatusFlags();
    telem_frame.seq++;
    XCP_UpdateMeasurements();
    Telemetry_Send(&telem_frame);
    FDIR_Update();
}
```

---

## 고급 기능

### 칼만 필터 기반 고도 융합
- **입력**:
  - MS5611 기압계 고도
- **출력**:
  - 필터링된 고도 (kf_alt_m)
  - 수직 속도 추정 (hkf.x[1])
- **발산 보호**: `KF_CheckDivergence()` - 공분산 > 10000 또는 NaN 시 리셋
- **구현**: `Core/Src/kalman.c`, `Core/Inc/kalman.h`

### 자세 추정 (SFLP)
- **입력**: LSM6DSV16X SFLP 쿼터니언 출력
- **출력**: Roll, Pitch (도)
- **변환**: Quaternion → Euler (atan2, asin)
- **구현**: `Core/Src/app.c` App_Loop() 내

### I2C 통신 (BSP 추상화)
```c
// Core/Inc/bsp.h
int32_t BSP_I2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);
int32_t BSP_I2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);
int32_t BSP_I2C3_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);
int32_t BSP_I2C3_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);
```

### UART 통신
```c
// Core/Src/usart.c
int32_t BSP_UART_Write(uint8_t *pData, uint16_t Len);
int32_t BSP_UART_Read(uint8_t *pData, uint16_t Len);
```

---

## 디버그 기능

### XCP 캘리브레이션 프로토콜
- **용도**: DAQ 측정 데이터 실시간 모니터링
- **구현**: `Core/Src/xcp.c`, `Core/Inc/xcp.h`
- **API**: `XCP_Init()`, `XCP_UpdateMeasurements()`

### FDIR 상태 플래그
```c
// Core/Inc/fdir.h
#define STATUS_SYS_OK         (1 << 0)
#define STATUS_GPS_WARN       (1 << 1)
#define STATUS_BARO_WARN      (1 << 2)
#define STATUS_IMU_WARN       (1 << 3)
#define STATUS_TEMP_WARN      (1 << 4)
#define STATUS_HEATER_ACTIVE  (1 << 5)
#define STATUS_LOW_BATTERY    (1 << 6)
#define STATUS_FDIR_RECOVERY  (1 << 7)
#define STATUS_ALT_JUMP       (1 << 8)
#define STATUS_RANGE_ERROR    (1 << 9)

uint16_t FDIR_GetStatusFlags(void);  // 텔레메트리에 포함
```

---

## 향후 개선 사항

### 권장 사항
1. **히터 PID 튜닝**: 실제 환경에서 Kp, Ki, Kd 최적화
2. **칼만 필터 파라미터**: 프로세스/측정 노이즈 공분산 조정
3. **전력 소모 최적화**: 센서 duty cycle 조정
4. **LoRa RF 설정**: SF, BW, Coding Rate 비행 프로파일 맞춤
5. **플래시 로깅**: 비행 데이터 온보드 저장 (옵션)

### 확장 가능성
- **추가 센서**: I2C/SPI/UART 포트 여유 있음
- **외부 플래시**: SPI 인터페이스 사용 가능
- **SD 카드 로깅**: SDIO/SPI 연결 가능

---

## 참조 문서

### 데이터시트
- [STM32G431xx Reference Manual](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- LSM6DSV16X, MLX90393, GDK101, DS18B20, XA1110, SEN0321, SHT31-D, MS5611, MCP9600, CM1107N, PMS3003

### 소프트웨어
- [STM32Cube HAL Documentation](https://www.st.com/en/embedded-software/stm32cubeg4.html)
- [CMake Build System](https://cmake.org/documentation/)

### 프로젝트 파일
- `reference/STM32G431/pin_mapping_stm32g431_2026-01-03.csv`
- `stm32_spaceballoon.ioc` (STM32CubeMX 프로젝트)

---

## HITL 시뮬레이션 환경

### 개요
실제 하드웨어(STM32)와 시뮬레이션 환경(PC/Mock)을 결합하여 비행 시나리오를 지상에서 검증하는 시스템입니다.

### 아키텍처
```
┌─────────────────────────────────┐      ┌──────────────────────────────┐
│  STM32G431 (Flight Computer)    │      │      ESP32 Bridge (Main)     │
│                                 │ UART │                              │
│ - 실행 코드: 실제 비행 펌웨어      │◄────►│ - 역할: STM32와 가상환경 연결    │
│ - 센서 드라이버: 실제 I2C/UART 사용 │      │ - 통신: ESP-NOW (무선 백플레인) │
└─────────────────────────────────┘      └──────────────┬───────────────┘
                                                        │ ESP-NOW
                                         ┌──────────────┴───────────────┐
                                         │                              │
                                  ┌──────▼──────┐                ┌──────▼──────┐
                                  │ I2C1 Mock   │                │ I2C3 Mock   │
                                  │ (ESP32)     │                │ (ESP32)     │
                                   ├─────────────┤                ├─────────────┤
                                   │ LSM6DSV16X  │                │ CM1107N     │
                                   │ MLX90393    │                │ MCP9600     │
                                   │ GDK101      │                │ SHT31-D     │
                                   │             │                │ MS5611      │
                                  └─────────────┘                └─────────────┘
```

### Mock 구성
1. **Main Control (Bridge)**: STM32의 UART 텔레메트리를 수신하고, 시뮬레이션 시나리오를 주입합니다.
2. **I2C1 Dual Mock**: Downside 보드의 센서(IMU, Mag, Baro, Env)를 모사하며, 실제 I2C 프로토콜로 STM32와 통신합니다.
3. **I2C3 Dual Mock**: Upside 보드의 센서(Gas, Temp)를 모사합니다.
4. **GPIO Mock**: 1-Wire 온도 센서 및 히터/리셋 라인 동작을 검증합니다.

### 검증 항목
- **FDIR 로직**: 센서 고장 주입(Timeout, Bad Value) 시 복구 동작 확인
- **통신 안정성**: 50Hz 텔레메트리 루프 및 I2C 버스 부하 테스트
- **미션 시퀀스**: 상승/하강 시나리오에 따른 알고리즘(KF, 히터) 동작 검증

---

## 버전 이력

| 버전 | 날짜 | 변경 사항 |
|------|------|-----------|
| Rev 3.5 | 2026-01-08 | 최종 검증: I2C1 SDA 핀(PB9), UART1 보레이트(115200), 센서 테이블, PID/KF/FDIR API, BSP 경로 |
| Rev 3.4 | 2026-01-08 | 아키텍처/핀맵 일치: App_Init/App_Loop, Bare-metal 구조, GPIO 핀 할당(LSM_RST/SHT_RST/GDK_RST) |
| Rev 3.3 | 2026-01-08 | 프레임 구조(version/msg_type/timestamp), I2C 버스 할당, 디렉토리 구조 |
| Rev 3.2 | 2026-01-08 | 텔레메트리 116바이트 페이로드, HITL/SITL/Python 일관성 |
| Rev 3.1 | 2026-01-06 | GPS UTC 시간 필드, Multi-GNSS 위성 카운트 |
| Rev 3.0 | 2026-01-03 | 구현 기반 사양서 재작성 |

---

**문서 작성**: 박현수
**프로젝트**: STM32 성층권 풍선 센서 플랫폼
**Repository**: C:\Users\hyuns\Desktop\stm32_spaceballoon
