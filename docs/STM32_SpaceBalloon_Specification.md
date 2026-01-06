# STM32 성층권 풍선 센서 플랫폼 사양서

**버전:** Rev 3.0 (구현 기준)
**날짜:** 2026-01-03
**MCU:** STM32G431CBU6 (Cortex-M4F @ 170MHz)
**RTOS:** FreeRTOS (Thread-Safe Strategy 4)

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
│  │         FreeRTOS Kernel                      │   │
│  ├──────────────────────────────────────────────┤   │
│  │  Services Layer                              │   │
│  │  - GPS Service                               │   │
│  │  - IMU/Mag/Sensors Services                  │   │
│  │  - Health Monitor Service                    │   │
│  │  - Heater Service (PID)                      │   │
│  │  - Telemetry Service (50Hz)                  │   │
│  │  - Altitude Kalman Filter                    │   │
│  ├──────────────────────────────────────────────┤   │
│  │  Drivers Layer                               │   │
│  │  - I2C Recovery, UART RX/TX, 1-Wire          │   │
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
| 플래시 | 128KB (사용: 52,440 bytes / 40.0%) |
| SRAM | 32KB (사용: 8,940 bytes / 27.3%) |
| FPU | fpv4-sp-d16 (hard float ABI) |

### 주변장치 사용
- **I2C1**: Downside 센서 보드 (LSM6DSV16X, MLX90393, GDK101, SEN0321, SHT31-D, MS5611)
- **I2C3**: Upside 센서 보드 (CM1107N, MCP9600)
- **UART1**: XA1110 GPS 모듈 (9600 baud)
- **UART2**: PMS3003 미세먼지 센서 (9600 baud)
- **UART3**: LoRa32 텔레메트리 출력
- **ADC1_IN2 (PA1)**: 배터리 전압 측정
- **TIM3_CH1 (PA6)**: 배터리 히터 PWM (Kapton 필름)
- **TIM8_CH1 (PC6)**: 보드 히터 PWM (Minibulb)
- **1-Wire (PB15)**: DS18B20 온도 센서 x2 (배터리 + 보드)

---

## 센서 구성

### 전체 센서 목록 (11종)

| # | 센서 모델 | 측정 항목 | 인터페이스 | 주소/설정 | 서비스 모듈 |
|---|-----------|-----------|------------|-----------|-------------|
| 1 | LSM6DSV16X | 6축 IMU (가속도/자이로) | I2C1 | 0x6B | sensors.c |
| 2 | MLX90393 | 3축 자기계 | I2C1 | 0x0C | mag_service.c |
| 3 | GDK101 | 방사선 (γ선) | I2C1 | 0x18 | gdk101_service.c |
| 4 | DS18B20 x2 | 온도 (배터리/보드) | 1-Wire (PB15) | - | aux_sensors_service.c |
| 5 | XA1110 | GPS | UART1 (9600) | NMEA | gps_service.c |
| 6 | SEN0321 | 오존 (O3) | I2C1 | 0x73 | ozone_service.c |
| 7 | SHT31-D | 온습도 | I2C1 | 0x44 | sht31_service.c |
| 8 | MS5611 | 기압/온도 | I2C1 | 0x77 | ms5611_service.c |
| 9 | MCP9600 | 열전대 (K-type) | I2C3 | 0x60 | mcp9600_service.c |
| 10 | CM1107N | CO2 농도 | I2C3 | 0x31 | co2_service.c |
| 11 | PMS3003 | 미세먼지 (PM1.0/2.5/10) | UART2 (9600) | - | pms3003_service.c |

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
- **리셋 제어**: PB13 (SHT_RST) - P-MOS (HIGH=OFF, LOW=ON)

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
- **핀**: PA14 (SDA), PA15 (SCL)
- **속도**: 400kHz (Fast Mode)
- **센서**: LSM6DSV16X, MLX90393, GDK101, SEN0321, SHT31-D, MS5611
- **복구 메커니즘**:
  - SCL 클럭 펄스 생성 (9회)
  - 소프트웨어 리셋 시퀀스
  - 구현: `drivers/i2c_recovery.c`

#### I2C3 (Upside Board)
- **핀**: PB5 (SDA), PA8 (SCL)
- **속도**: 400kHz (Fast Mode)
- **센서**: CM1107N, MCP9600
- **복구 메커니즘**: I2C1과 동일

### UART 구성

#### UART1 (GPS)
- **핀**: PC4 (TX), PA10 (RX)
- **속도**: 9600 baud, 8N1
- **DMA**: RX DMA 사용 (DMA1_Channel1)
- **파싱**: `services/nmea_parser.c`

#### UART2 (PMS3003)
- **핀**: PA2 (TX), PA3 (RX)
- **속도**: 9600 baud, 8N1
- **DMA**: RX DMA 사용 (DMA1_Channel3)
- **파싱**: `drivers/pms_parser.c`

#### UART3 (텔레메트리)
- **핀**: PC10 (TX), PC11 (RX)
- **속도**: 설정 가능 (LoRa32 모듈 연결)
- **용도**: 텔레메트리 데이터 전송

### 1-Wire (DS18B20)
- **핀**: PB15
- **모드**: 비트뱅 (GPIO 재구성)
  - LOW 출력: Open-drain
  - 읽기: Input pull-up
- **타이밍**: 표준 1-Wire 프로토콜
- **구현**: `drivers/ds18b20.c`

---

## 전원 관리

### 센서 전원 제어 (P-MOS 기반)

각 센서/모듈마다 개별 P-MOS 스위치(AO3401F)를 통한 전원 제어:

| 센서 | 리셋 라인 | GPIO 핀 | 초기 상태 | 제어 |
|------|-----------|---------|-----------|------|
| LSM6DSV16X | LSM_RST | PB11 | LOW | reset_lines.c |
| MLX90393 | MLX_RST | PB14 | HIGH | reset_lines.c |
| CM1107N | CO2_RST | PB0 | LOW | reset_lines.c |
| MS5611 | MS_RST | PA5 | LOW | reset_lines.c |
| MCP9600 | MCP_RST | PA4 | LOW | reset_lines.c |
| SHT31-D | SHT_RST | PB13 | - | reset_lines.c |
| PMS3003 | PMS_SET | PB10 | HIGH | reset_lines.c |
| Sensor Board | SEN_RST | PB1 | - | reset_lines.c |

### 전원 제어 API
```c
// drivers/reset_lines.h
bool reset_line_pulse(reset_line_t line, uint32_t low_ms, uint32_t high_ms, uint32_t low2_ms);
bool reset_line_set(reset_line_t line, bool level_high);
bool reset_line_pulse_high_low_high(reset_line_t line, uint32_t high_ms, uint32_t low_ms, uint32_t high2_ms);
```

### 배터리 모니터링
- **ADC 채널**: ADC1_IN2 (PA1)
- **분압비**: 2:1 (설정: BAT_DIVIDER_NUM=2, BAT_DIVIDER_DEN=1)
- **출력**: mV (uint16_t)
- **구현**: `drivers/battery_adc.c`

---

## 헬스 모니터링

### 감시 메커니즘
- **체크 주기**: 100ms
- **타임아웃 임계값**:
  - 일반 센서: 2000ms
  - PMS3003: 3000ms (느린 응답 고려)
- **복구 시도**: 최대 5회
- **구현**: `services/health_monitor_service.c`

### 감시 대상
```c
typedef struct {
    bool ever_updated;      // 최소 1회 업데이트 여부
    uint8_t attempts;       // 복구 시도 횟수
    bool permfail;          // 영구 실패 플래그
} health_state_t;
```

각 센서별 상태 추적:
- GPS (UART1)
- PMS3003 (UART2)
- I2C1 센서들 (LSM, MLX, GDK, SEN, SHT, MS)
- I2C3 센서들 (CO2, MCP)
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
// services/heater_service.c
float error = target_temp - current_temp;
float p_term = Kp * error;
float i_term = Ki * integral_err;  // Anti-windup 적용
float d_term = Kd * (error - last_error) / dt;
float output = p_term + i_term + d_term;  // PWM duty 출력
```

### 제어 주기
- **업데이트 주기**: 1Hz (1000ms)
- **안전 장치**: 온도 센서 실패 시 히터 자동 OFF

### API
```c
// services/heater_service.h
void heater_bat_set_target(float temp_c);
float heater_bat_get_target(void);
float heater_bat_get_duty(void);

void heater_board_set_target(float temp_c);
float heater_board_get_target(void);
float heater_board_get_duty(void);
```

---

## 텔레메트리 시스템

### 전송 사양
- **전송 속도**: 50Hz (20ms 간격)
- **동기화**: GPS 1PPS 신호 기준
- **프로토콜**: 바이너리 프레이밍
- **CRC**: CRC16 체크섬
- **출력**: UART3 → LoRa32 모듈

### 프레임 구조
```c
// services/telemetry_frame.h
typedef struct __attribute__((packed)) {
    uint8_t magic[2];           // 0xA5, 0x5A
    uint16_t seq;               // 시퀀스 번호
    uint16_t payload_len;       // 페이로드 길이
    telemetry_payload_sensor_snapshot_t payload;
    uint16_t crc16;             // CRC16 체크섬
} telemetry_frame_t;
```

### 센서 스냅샷 페이로드
```c
typedef struct __attribute__((packed)) {
    // 시스템 상태
    uint32_t uptime_ms;
    uint16_t status_flags;
    uint16_t bat_mv;

    // IMU (가속도, 자이로)
    int32_t accel_mps2_x1000[3];      // x, y, z (m/s² x1000)
    int32_t gyro_rads_x1000[3];       // x, y, z (rad/s x1000)

    // 자기계
    float mag_uT[3];                  // x, y, z (µT)

    // 온도 측정
    int16_t indoor_2nd_temp_c_x100;   // MCP9600 (°C x100)
    int16_t external_temp_c_x100;     // 외부 온도
    int16_t sht31_temp_c_x100;        // SHT31 온도
    int16_t bat_temp_c_x100;          // 배터리 온도 (DS18B20)
    int16_t board_temp_c_x100;        // 보드 온도 (DS18B20)
    int16_t ms5611_temp_c_x100;       // MS5611 온도

    // GPS 데이터
    int32_t gps_lat_deg_e7;           // 위도 (도 x 10^7)
    int32_t gps_lon_deg_e7;           // 경도 (도 x 10^7)
    float gps_alt_m;                  // GPS 고도 (m)
    uint8_t gps_fix;                  // Fix 상태 (0=No, 1=2D, 2=3D)
    uint8_t gps_sats_used;            // 사용 중인 위성 수
    uint8_t gps_sats_in_view_total;   // 총 가시 위성
    uint8_t gps_sats_in_view_gps;
    uint8_t gps_sats_in_view_glonass;
    uint8_t gps_sats_in_view_galileo;
    uint8_t gps_sats_in_view_beidou;

    // 대기질
    uint16_t co2_ppm;                 // CO2 (ppm)
    int16_t ozone_ppb;                // 오존 (ppb)
    uint16_t pm1_ugm3;                // PM1.0 (µg/m³)
    uint16_t pm25_ugm3;               // PM2.5 (µg/m³)
    uint16_t pm10_ugm3;               // PM10 (µg/m³)

    // 기압/습도
    uint32_t ms5611_press_pa;         // 기압 (Pa)
    uint16_t sht31_rh_x100;           // 습도 (%RH x100)

    // 방사선
    uint16_t gdk101_usvh_x100;        // 선량율 (µSv/h x100)

    // 히터 상태
    uint8_t heater_bat_duty_percent;  // 배터리 히터 듀티 (%)
    uint8_t heater_board_duty_percent;// 보드 히터 듀티 (%)

    // 고도 추정
    float press_alt_m;                // 기압 고도 (m)
    float kf_alt_m;                   // 칼만 필터 고도 (m)
    float kf_roll_deg;                // Roll (도)
    float kf_pitch_deg;               // Pitch (도)

    // 예약 필드
    int16_t reserved1;
    uint8_t reserved2;
    uint8_t reserved3;
    uint8_t reserved4;
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
- **서비스**: `services/telemetry_service.c`
- **프레이밍**: `services/telemetry_frame.c`
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
| PA14 | I2C1_SDA | AF OD | I2C1 데이터 | Downside 센서 |
| PA15 | I2C1_SCL | AF OD | I2C1 클럭 | Downside 센서 |
| **포트 B** |
| PB0 | CO2_RST | GPIO Output | CO2 센서 리셋 | CM1107N RST |
| PC6 | TIM8_CH1 | AF PP | 보드 히터 PWM | Minibulb |
| PB4 | GPS_PPS | EXTI4 (Rising) | GPS 1PPS 입력 | XA1110 1PPS |
| PB5 | I2C3_SDA | AF OD | I2C3 데이터 | CM1107N, MCP9600 |
| PB6 | LSM_INT | EXTI6 (Rising) | IMU 인터럽트 | LSM6DSV16X INT |
| PB7 | MLX_INT | EXTI7 (Rising) | 자기계 인터럽트 | MLX90393 INT |
| PB10 | PMS_SET | GPIO Output | PMS3003 제어 | PMS3003 SET |
| PB11 | LSM_RST | GPIO Output | IMU 리셋 | LSM6DSV16X RST |
| PB12 | GPS_INT | EXTI12 (Rising) | GPS 인터럽트 | XA1110 INT |
| PB13 | SHT_RST | GPIO Output | SHT31 리셋 | SHT31 RST |
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

### 컴파일 플래그
```ini
[env:genericSTM32G431CB]
platform = ststm32
board = genericSTM32G431CB
framework = stm32cube
extra_scripts = pre:platformio_build.py

build_unflags = -mfpu=fpv5-sp-d16

build_flags =
    -DBAT_DIVIDER_NUM=2
    -DBAT_DIVIDER_DEN=1
    -DUSE_HAL_DRIVER
    -DSTM32G431xx
    -DSTM32_THREAD_SAFE_STRATEGY=4
    -IMiddlewares/Third_Party/FreeRTOS/Source/include
    -IMiddlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    -IMiddlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
```

### FPU 설정
```python
# platformio_build.py
env.Append(
    CCFLAGS=[
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=hard"
    ],
    LINKFLAGS=[
        "-mfpu=fpv4-sp-d16",
        "-mfloat-abi=hard"
    ]
)
```

### 디렉토리 구조
```
stm32_spaceballoon/
├── Core/
│   ├── Inc/
│   │   ├── app/
│   │   │   └── board_pins.h         # 핀 매핑 정의
│   │   ├── drivers/                 # 드라이버 헤더
│   │   ├── services/                # 서비스 헤더
│   │   ├── main.h
│   │   ├── FreeRTOSConfig.h
│   │   └── stm32g4xx_hal_conf.h
│   └── Src/
│       ├── app/
│       │   └── app.c                # 애플리케이션 진입점
│       ├── drivers/                 # 하드웨어 드라이버
│       ├── services/                # 고수준 서비스
│       ├── main.c                   # HAL 초기화
│       └── stm32g4xx_it.c           # 인터럽트 핸들러
├── Drivers/
│   ├── CMSIS/
│   └── STM32G4xx_HAL_Driver/
├── Middlewares/
│   └── Third_Party/
│       └── FreeRTOS/
├── reference/
│   └── STM32G431/
│       └── pin_mapping_stm32g431_2026-01-03.csv
├── platformio.ini                   # PlatformIO 설정
├── platformio_build.py              # 빌드 스크립트
├── STM32G431CBUx_FLASH.ld          # 링커 스크립트
└── Makefile                         # GNU Make 지원
```

### 빌드 명령
```bash
# PlatformIO 빌드
pio run

# 디버그 환경 빌드
pio run -e genericSTM32G431CB_debug

# 업로드
pio run -t upload

# 클린
pio run -t clean
```

### 메모리 사용량 (빌드 결과)
```
RAM:   [===       ]  27.3% (사용: 8940 bytes, 전체: 32768 bytes)
Flash: [====      ]  40.0% (사용: 52440 bytes, 전체: 131072 bytes)
```

---

## 소프트웨어 아키텍처

### 서비스 초기화 순서
```c
// Core/Src/app/app.c - app_init()
void app_init(void) {
    gps_service_init();           // GPS UART 초기화
    aux_sensors_service_init();   // DS18B20, 배터리 ADC
    air_quality_service_init();   // PMS3003
    co2_service_init();           // CM1107N
    ozone_service_init();         // SEN0321
    heater_service_init();        // 히터 PID 제어
    gdk101_service_init();        // 방사선 센서
    sht31_service_init();         // 온습도 센서
    ms5611_service_init();        // 기압 센서
    imu_service_init();           // LSM6DSV16X
    mag_service_init();           // MLX90393
    mcp9600_service_init();       // 열전대
    alt_kf_service_init();        // 칼만 필터
    uart4_debug_log_init();       // 디버그 로그
    swd_debug_probe_init();       // SWD 프로브
    health_monitor_service_init();// 헬스 모니터
    telemetry_service_init();     // 텔레메트리
}
```

### 메인 루프 실행 순서
```c
// Core/Src/app/app.c - app_tick()
void app_tick(uint32_t now_ms) {
    // 지연 민감 경로 우선
    gps_service_tick(now_ms);
    imu_service_tick(now_ms);
    uart4_debug_log_tick(now_ms);
    swd_debug_probe_tick(now_ms);
    telemetry_service_tick(now_ms);

    // 센서 / 느린 서비스 (I2C/UART 블로킹 가능)
    aux_sensors_service_tick(now_ms);
    ms5611_service_tick(now_ms);
    sht31_service_tick(now_ms);
    mcp9600_service_tick(now_ms);
    mag_service_tick(now_ms);
    gdk101_service_tick(now_ms);
    co2_service_tick(now_ms);
    ozone_service_tick(now_ms);
    air_quality_service_tick(now_ms);
    heater_service_tick(now_ms);
    alt_kf_service_tick(now_ms);

    // 헬스 모니터 마지막 (최신 타임스탬프 관찰)
    health_monitor_service_tick(now_ms);
}
```

---

## 고급 기능

### 칼만 필터 기반 고도 융합
- **입력**:
  - MS5611 기압계 고도
  - XA1110 GPS 고도
- **출력**:
  - 융합 고도 (kf_alt_m)
  - 자세 추정 (roll, pitch)
- **구현**: `services/alt_kf_service.c`

### I2C 버스 복구
```c
// drivers/i2c_recovery.c
bool i2c_recovery_attempt(I2C_TypeDef *i2c);
```
- SCL 라인에 9개 클럭 펄스 생성
- SDA 라인 상태 확인
- I2C 주변장치 재초기화

### UART RX 폴링
```c
// drivers/uart_rx_poll.c
uint16_t uart_rx_poll(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t capacity);
```
- DMA 기반 원형 버퍼
- 데이터 손실 없는 수신

### NMEA 파서
```c
// services/nmea_parser.c
bool nmea_parse_line(const char *line, nmea_sentence_t *out);
```
- GGA, GSA, GSV, RMC, VTG 메시지 지원
- CRC 검증

### DWT 고정밀 딜레이
```c
// drivers/dwt_delay.c
void dwt_delay_us(uint32_t us);
```
- Cortex-M4 DWT 사이클 카운터 사용
- µs 단위 정밀 딜레이

---

## 디버그 기능

### SWD 디버그 프로브
- **활성화**: `SWD_DEBUG_PROBE_ENABLE=1` 빌드 플래그
- **용도**: Watch/Live Expressions (지상 디버그 전용)
- **구현**: `services/swd_debug_probe.c`
- **주의**: 비행 시 비활성화 (기본 환경에서는 비활성)

### UART4 디버그 로그
- **용도**: 실시간 디버그 메시지 출력
- **구현**: `services/uart4_debug_log.c`

### 건강 상태 플래그
```c
// services/health_monitor_service.h
typedef struct {
    uint32_t sensor_health_flags;  // 비트 플래그
    uint32_t ext_flags;            // 확장 플래그
} health_status_t;
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
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [PlatformIO STM32 Platform](https://docs.platformio.org/en/latest/platforms/ststm32.html)

### 프로젝트 파일
- `reference/STM32G431/pin_mapping_stm32g431_2026-01-03.csv`
- `stm32_spaceballoon.ioc` (STM32CubeMX 프로젝트)

---

## 버전 이력

| 버전 | 날짜 | 변경 사항 |
|------|------|-----------|
| Rev 3.0 | 2026-01-03 | 구현 기반 사양서 재작성, PlatformIO 빌드 완료 |
| Rev 2.2 | - | 원본 사양서 (PDF) |

---

**문서 작성**: Claude Code
**프로젝트**: STM32 성층권 풍선 센서 플랫폼
**Repository**: C:\Users\hyuns\Desktop\stm32_spaceballoon
