# STM32 Space Balloon Radiosonde

고고도 기상관측용 라디오존데 펌웨어 - STM32G431CBU6 기반

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-STM32G4-green.svg)
![Simulation](https://img.shields.io/badge/simulation-HostSim%20%7C%20HITL-orange.svg)
![Telemetry](https://img.shields.io/badge/telemetry-116%20bytes-purple.svg)

---

## 📋 목차

- [개요](#-개요)
- [개발 환경](#-개발-환경)
- [하드웨어 구성](#-하드웨어-구성)
- [프로젝트 구조](#-프로젝트-구조)
- [빌드 방법](#-빌드-방법)
  - [실제 하드웨어 빌드](#실제-하드웨어-빌드)
  - [호스트 시뮬레이션 빌드](#호스트-시뮬레이션-빌드)
- [테스트 및 실행](#-테스트-및-실행)
- [HITL 시뮬레이션](#-hitl-시뮬레이션)
- [텔레메트리 프로토콜](#-텔레메트리-프로토콜)
- [센서 목록](#-센서-목록)

---

## 🎈 개요

이 프로젝트는 고고도 기구(HAB: High Altitude Balloon)에 탑재되는 라디오존데 시스템입니다.

**주요 기능:**

- 11종 센서 데이터 수집 (IMU, GPS, 기압, 온도, 습도, 대기질, 방사선 등)
- Kalman 필터 기반 자세/고도 추정
- PID 제어 기반 히터 시스템 (배터리/보드 온도 유지)
- LoRa 텔레메트리 전송
- FDIR(Fault Detection, Isolation, Recovery) 시스템

---

## 💻 개발 환경

### IDE / 에디터

| 도구 | 버전 | 용도 |
| :--- | :--- | :--- |
| **VS Code** | 1.96+ | 메인 개발 환경 |
| **STM32CubeMX** | 6.x | 핀 설정 및 코드 생성 |
| **STM32CubeIDE** | 1.x | (선택) CubeMX 통합 IDE |

### VS Code 확장

| 확장 | 설명 |
| :--- | :--- |
| **CMake Tools** | CMake 빌드 시스템 지원 |
| **Cortex-Debug** | ARM Cortex-M 디버깅 |
| **C/C++** (Microsoft) | IntelliSense 및 코드 탐색 |
| **clangd** | 코드 분석 및 자동완성 |

### 툴체인 / 빌드 도구

| 도구 | 버전 | 용도 |
| :--- | :--- | :--- |
| **ARM GCC** | 13.x+ | 크로스 컴파일러 (arm-none-eabi-gcc) |
| **cube-cmake** | - | STM32 CMake 빌드 도구 |
| **MinGW-w64** | 15.x | Windows 네이티브 빌드 (시뮬레이션) |
| **OpenOCD** | 0.12+ | 플래시/디버깅 (xpack 버전 포함) |
| **Python** | 3.10+ | 스크립트 및 데이터 변환 |

### 디버거 / 프로그래머

| 도구 | 용도 |
| :--- | :--- |
| **ST-Link V2** | STM32 플래싱 및 SWD 디버깅 |
| **OpenOCD** | GDB 서버 |

### 설치 명령 (Windows)

```powershell
# Scoop 사용 시
scoop install gcc arm-none-eabi-gcc cmake python

# 또는 수동 설치
# - ARM GCC: https://developer.arm.com/downloads/-/gnu-rm
# - MinGW-w64: https://www.mingw-w64.org/
# - cube-cmake: STMicroelectronics 제공
# - OpenOCD: https://xpack.github.io/openocd/
```

### VS Code 설정 (권장)

`.vscode/settings.json`:

```json
{
    "cmake.configureOnOpen": true,
    "cmake.generator": "MinGW Makefiles",
    "cortex-debug.openocdPath": "${workspaceFolder}/xpack-openocd-0.12.0-4/openocd/bin/openocd.exe"
}
```

---

## 🔧 하드웨어 구성

| 구분 | 센서/모듈 | 인터페이스 | 용도 |
| :--- | :--- | :--- | :--- |
| MCU | STM32G431CBU6 | - | 메인 프로세서 |
| IMU | LSM6DSV16X | I2C1 | 가속도/자이로 |
| Mag | MLX90393 | I2C1 | 자기장 |
| Baro | MS5611 | I2C3 | 기압/온도 |
| Temp/Hum | SHT31-D | I2C3 | 온습도 |
| GPS | XA1110 | UART1 | 위치/고도 |
| Radiation | GDK101 | I2C1 | 방사선량 |
| CO2 | CM1107N | I2C3 | CO2 농도 (UART→I2C3 변경) |
| Ozone | SEN0321 | I2C3 | 오존 농도 |
| PM | PMS3003 | UART3 | 미세먼지 |
| Thermo | MCP9600 | I2C3 | 외부 온도 |
| 1-Wire | DS18B20 x2 | GPIO | 보드/배터리 온도 |

---

## 📁 프로젝트 구조

```text
stm32_spaceballoon/
├── Core/
│   ├── Inc/                    # 헤더 파일
│   │   ├── app.h
│   │   ├── sensors.h
│   │   ├── telemetry.h
│   │   ├── kalman.h
│   │   ├── pid.h
│   │   └── fdir.h
│   ├── Src/                    # 소스 파일
│   │   ├── app.c               # 메인 애플리케이션 로직
│   │   ├── sensors.c           # 센서 드라이버 통합
│   │   ├── telemetry.c         # 텔레메트리 패킹/CRC
│   │   ├── kalman.c            # Kalman 필터
│   │   ├── pid.c               # PID 제어기
│   │   └── fdir.c              # 오류 감지/복구
│   └── Drivers/                # 개별 센서 드라이버
│       ├── lsm6dsv16x/
│       ├── mlx90393/
│       ├── ms5611/
│       └── ...
├── RENODE_TEST(HIL)/           # Renode 시뮬레이션 환경 (HIL/SITL)
│   ├── renode/                 # Renode 플랫폼 및 스크립트
│   ├── run_fdir_tests.py       # 자동화된 FDIR 테스트 러너
│   └── results/                # 테스트 결과 및 리포트
├── SITL/                       # Software-In-The-Loop 시뮬레이션
│   ├── HostSim/                # 호스트 PC 시뮬레이션 (C 언어 스텁)
│   ├── test/                   # 알고리즘 유닛 테스트 (Kalman, PID 등)
│   ├── simulation_reference_data/ # 실제 비행 데이터 (RS41)
│   ├── build_host/             # HostSim 빌드 결과물
│   └── build_test/             # Unit Test 빌드 결과물
├── HITL/                       # Hardware-In-The-Loop 시나리오 및 대시보드
├── cmake/
│   └── gcc-arm-none-eabi.cmake # 툴체인 설정
├── CMakeLists.txt              # 메인 빌드 설정
├── CMakePresets.json           # Debug/Release 프리셋
└── stm32_spaceballoon.ioc      # CubeMX 설정
```

---

## 🛠 빌드 방법

### 요구사항

- **실제 하드웨어**: ARM GCC Toolchain, cube-cmake, OpenOCD
- **시뮬레이션**: MinGW-w64, cube-cmake, Python 3.x

### 실제 하드웨어 빌드

```powershell
# 1. Debug 프리셋 선택 (VS Code에서)
# Ctrl+Shift+P → CMake: Select Configure Preset → Debug

# 2. 빌드
cube-cmake --build build/Debug

# 3. 플래시
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program build/Debug/stm32_spaceballoon.elf verify reset exit"
```

### 호스트 시뮬레이션 빌드 (SITL)

```powershell
cd SITL/HostSim

# 1. (선택) 비행 데이터 업데이트
python convert_flight_data.py

# 2. CMake 설정
cube-cmake -B ../build_host -G "MinGW Makefiles"

# 3. 빌드
mingw32-make -C ../build_host
```

> [!IMPORTANT]
> 폴더 이동으로 인해 기존의 `CMakeCache.txt`가 무효화되었습니다.
> 통합된 `SITL/build_host` 와 `SITL/build_test`에서 다시 빌드하실 때, **기존 빌드 폴더를 삭제하거나 `CMakeCache.txt`를 제거**한 후 다시 생성해야 정상적으로 빌드됩니다.

---

## 🛡️ 코드 안전성 (MISRA C:2012)

이 펌웨어는 **MISRA C:2012** 가이드라인을 준수하여 작성되었습니다.
- **스택 안전성:** 재귀 호출 제거, 스택 사용량 최소화
- **타입 안전성:** 명시적인 타입 캐스팅 및 크기 지정 (`uint8_t`, `int32_t` 등)
- **알고리즘 검증:** Kalman Filter 및 PID 제어기의 수치적 안정성(NaN/Inf 체크) 강화

---

## 🧪 테스트 및 실행

### 1. 유닛 테스트 (Unit Tests)
개별 모듈의 알고리즘 동작을 검증합니다.

```powershell
# 예: Kalman Filter 테스트
cd SITL/test/test_kalman
gcc -o test_kalman test_kalman.c ..\..\..\Core\Src\kalman.c -I..\..\..\Core\Inc -DHOST_TEST_MODE
.\test_kalman.exe
```

### 2. 통합 테스트 (Integration Test)
전체 시스템의 비행 시나리오(대기-상승-폭발-하강)를 시뮬레이션하여 데이터 로직을 검증합니다.

```powershell
cd SITL/test/test_integration
# 빌드 및 실행 (gcc 필요)
gcc -o mission_test.exe test_mission.c ... (상세 명령어는 SITL/walkthrough.md 참조)
.\mission_test.exe
```

### 3. 호스트 시뮬레이션 (HostSim)
과거 비행 데이터(RS41)를 재생하여 실시간 텔레메트리 전송을 검증합니다.

```powershell
cd SITL/HostSim
..\build_host\test_host.exe
```
**출력 예시:**
```text
[Mock] Sensors Initialized - RS41 Flight Data Mode
[Mock] Loaded 65 flight data points
[Mock] Altitude range: 5174m - 5631m

=== TELEMETRY FRAME ===
Frame Size: 130 bytes
Header: magic=A5 5A, ver=1, type=0x02, seq=1, ts=20ms
--- Sensor Payload ---
GPS: 35.0929800N, 126.9988700E, Alt=5174.3m, Fix=1, Sats=9/12
Battery: 2800 mV
Pressure: 52700 Pa, Humidity: 24.00%
CRC16: 0xABCD
===========================
```

### 실제 하드웨어 디버깅

```powershell
# OpenOCD 서버 시작 (터미널 1)
.\xpack-openocd-0.12.0-4\openocd\bin\openocd.exe -f interface/stlink.cfg -f target/stm32g4x.cfg

# VS Code에서 F5 → "Cortex Debug" 선택
```

---

## 🚀 HITL 시뮬레이션

**HITL (Hardware-In-The-Loop)** 테스트를 통해 실제 STM32와 ESP32 Mock 보드를 연결하여 비행 시나리오를 검증합니다.

### HITL 구성
- **Main Control (ESP32-C3)**: PC와 통신, ESP-NOW 브로드캐스트
- **I2C Mock Nodes (4개)**: 센서 에뮬레이션 (LSM6DSV16X, GPS, 기압계 등)
- **Python Dashboard**: `HITL/sensor_sender.py`

### 실행 방법
```powershell
cd HITL
python sensor_sender.py
```

> 자세한 내용은 [HITL/README.md](HITL/README.md) 참조

---

## 🤖 Renode FDIR 시뮬레이션 (SITL)

**Renode**를 사용하여 하드웨어 없이 펌웨어의 전체 로직과 FDIR(장애 감지 및 복구) 기능을 시뮬레이션합니다.

- **테스트 항목**: 22개 (모든 I2C/UART 센서, 시스템 타이머, ADC, EXTI 등)
- **주입 장애**: I2C 버스 타임아웃, 센서 연결 해제, UART 통신 불량 등
- **검증 방법**: 자동화된 Python 스크립트를 통한 펌웨어 상태 및 복구 동작 상시 모니터링

### 실행 방법
```powershell
cd "RENODE_TEST(HIL)"
python run_fdir_tests.py
```

> 최근 테스트 결과: **22/22 PASS (100%)** - [상세 리포트 보기](RENODE_TEST(HIL)/results/README.md)

---

## 📡 텔레메트리 프로토콜

### 프레임 크기
| 구성 요소 | 크기 |
|----------|------|
| **페이로드** | 116 바이트 |
| **헤더** | 12 바이트 |
| **CRC** | 2 바이트 |
| **전체 프레임** | **130 바이트** |

### 프레임 및 페이로드 구조 (C Struct)

`Core/Inc/telemetry.h`에 정의된 패킷 구조체입니다. (`#pragma pack(1)` 적용됨)

```c
// 1. 센서 스냅샷 페이로드
typedef struct {
    /* 1. System Status */
    uint32_t uptime_ms;
    uint16_t status_flags;
    uint16_t co2_ppm;           /* CM1107N */

    /* 2. IMU (LSM6DSV16X) (x1000 scaled) */
    int32_t accel_mps2_x1000[3];
    int32_t gyro_rads_x1000[3];

    /* 3. Magnetometer (MLX90393) */
    float mag_uT[3];

    /* 4. Temperature (x100 scaled) */
    int16_t board_temp_c_x100;      /* DS18B20 (Board) */
    int16_t external_temp_c_x100;   /* MCP9600 */
    int16_t sht31_temp_c_x100;      /* SHT31 */

    /* 5. Reserved / Extended Status */
    int16_t bat_temp_c_x100;        /* DS18B20 (Battery) */

    /* 6. GPS (XA1110) (x10^7 scaled for lat/lon) */
    int32_t gps_lat_deg_e7;
    int32_t gps_lon_deg_e7;
    float gps_alt_m;
    uint8_t gps_fix;
    uint8_t gps_sats_used;
    uint8_t gps_sats_in_view_total;
    uint8_t gps_sats_in_view_gps;
    uint8_t gps_sats_in_view_glonass;
    uint8_t gps_sats_in_view_galileo;
    uint8_t gps_sats_in_view_beidou;
    
    /* GPS UTC Time (from RMC sentence) */
    uint8_t gps_utc_hour;
    uint8_t gps_utc_min;
    uint8_t gps_utc_sec;
    uint8_t gps_utc_day;
    uint8_t gps_utc_month;
    uint16_t gps_utc_year;

    uint16_t bat_mv;            /* ADC PA1 */

    /* 7. Air Quality */
    uint16_t pm1_ugm3;          /* PMS3003 */
    uint16_t pm25_ugm3;
    uint16_t pm10_ugm3;
    int16_t ozone_ppb;          /* SEN0321 */

    /* 8. Pressure / Humidity */
    uint16_t sht31_rh_x100;     /* SHT31 */
    uint32_t ms5611_press_pa;   /* MS5611 */
    int16_t ms5611_temp_c_x100;

    /* 9. Radiation */
    uint16_t gdk101_usvh_x100;  /* GDK101 */

    /* 10. Heater Status */
    uint8_t heater_bat_duty_percent;
    uint8_t heater_board_duty_percent;

    /* 11. Altitude Fusion */
    float press_alt_m;
    float kf_alt_m;
    float kf_roll_deg;
    float kf_pitch_deg;

} telemetry_payload_sensor_snapshot_t;

// 2. 전체 전송 프레임 (Header + Payload + CRC)
typedef struct {
    uint8_t magic[2];      /* {0xA5, 0x5A} */
    uint8_t version;       /* 1 */
    uint8_t msg_type;      /* 0x01 heartbeat, 0x02 sensor snapshot */
    uint16_t payload_len;  /* bytes */
    uint16_t seq;
    uint32_t timestamp_ms;
    telemetry_payload_sensor_snapshot_t payload;
    uint16_t crc16;        /* CRC-16/CCITT-FALSE over header+payload */
} telemetry_frame_t;
```

### 페이로드 상세 항목 (참고)
---

## 🌡 센서 목록

| ID | 센서 | 측정값 | 샘플링 |
| :--- | :--- | :--- | :--- |
| 0 | LSM6DSV16X | 가속도, 자이로 | 480Hz |
| 1 | MLX90393 | 자기장 X/Y/Z | 50Hz |
| 2 | MS5611 | 기압, 온도 | 5Hz |
| 3 | SHT31-D | 온도, 습도 | 1Hz |
| 4 | XA1110 | GPS 위치 | 1Hz |
| 5 | GDK101 | 방사선량 | 1Hz |
| 6 | CM1107N | CO2 | 1Hz (I2C3) |
| 7 | SEN0321 | 오존 | 1Hz |
| 8 | PMS3003 | 미세먼지 | 1Hz |
| 9 | MCP9600 | 열전대 온도 | 1Hz |
| 10 | DS18B20 | 1-Wire 온도 | 1Hz |

---

## 📄 라이선스

MIT License

---

## 🙏 감사의 글

- RS41 라디오존데 데이터: [SondeHub](https://sondehub.org/)
- STM32 HAL: STMicroelectronics
- LSM6DSV16X 드라이버: ST MEMS Drivers

---

**마지막 업데이트:** 2026-01-11
