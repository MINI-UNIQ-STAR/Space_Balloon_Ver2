# Space Balloon Ver2 - Zephyr RTOS Port

Zephyr RTOS 기반 고고도 풍선 라디오존데 펌웨어

## 📁 프로젝트 구조

```
zephyr_app/
├── CMakeLists.txt          # Zephyr 빌드 설정
├── prj.conf                 # Kconfig 설정
├── README.md                # 이 파일
├── .gitignore               # Git 무시 파일
├── boards/                  # 커스텀 보드 정의
│   ├── nucleo_g431rb.overlay
│   └── st/space_balloon_ver2/
├── docker/                  # Docker 빌드 환경
│   ├── Dockerfile
│   ├── build.sh
│   ├── flash.sh
│   ├── export.sh
│   └── README.md
├── include/
│   └── fdir_zephyr.h
└── src/
    ├── main.c              # 메인 진입점
    ├── fdir_zephyr.c       # FDIR 모듈
    ├── kalman.c            # 칼만 필터
    ├── pid.c               # PID 제어기
    ├── sensors_zephyr.c    # 센서 관리
    ├── telemetry_zephyr.c  # 텔레메트리 (UART3)
    └── actuators_zephyr.c  # 액추에이터
```

## 🎯 빌드 타겟

| 타겟 | 용도 | Flash | RAM |
|------|------|-------|-----|
| `qemu_cortex_m3` | QEMU 시뮬레이션 | 52KB (20%) | 13KB (21%) |
| `weact_stm32g431_core` | weact STM32G431CBU6 | 53KB (40%) | 14KB (43%) |
| `nucleo_g431rb` | Nucleo-G431RB 보드 | 53KB (40%) | 14KB (43%) |

> **참고**: QEMU에서는 실제 센서 하드웨어가 없어서 고정값이 출력됩니다. 실제 하드웨어에서는 센서 데이터가 정상적으로 출력됩니다.

---

## 🚀 설치 방법

### 방법 1: Docker 사용 (권장)

#### 1. Docker 이미지 빌드

```bash
cd zephyr_app/docker
docker build -t space-balloon-ver2:latest .
```

#### 2. 컨테이너 실행

```bash
# 일반 모드
docker run -it --rm space-balloon-ver2:latest

# USB 플래시 모드 (ST-Link 연결 시)
docker run -it --rm --privileged -v /dev/bus/usb:/dev/bus/usb space-balloon-ver2:latest
```

#### 3. 다른 컴퓨터로 전송

```bash
# 이미지 내보내기 (~1GB)
cd zephyr_app/docker
./export.sh

# 생성된 파일: space-balloon-ver2-docker.tar.gz
```

#### 4. 다른 컴퓨터에서 import

```bash
docker load < space-balloon-ver2-docker.tar.gz
docker run -it --rm space-balloon-ver2:latest
```

---

### 방법 2: 직접 설치

#### 1. 시스템 요구사항

- Ubuntu 22.04 / Windows 10+ / macOS
- Python 3.8+
- Git
- CMake 3.20+

#### 2. Zephyr SDK 설치

```bash
# Linux/macOS
wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64.tar.xz
tar xf zephyr-sdk-0.16.8_linux-x86_64.tar.xz
mv zephyr-sdk-0.16.8 ~/zephyr-sdk
~/zephyr-sdk/setup.sh
```

#### 3. Zephyr 프로젝트 초기화

```bash
# 가상환경 생성
python3 -m venv ~/zephyrproject/.venv
source ~/zephyrproject/.venv/bin/activate

# Zephyr 설치
pip install west
west init ~/zephyrproject
cd ~/zephyrproject
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
```

#### 4. 환경 변수 설정

```bash
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk
export ZEPHYR_BASE=~/zephyrproject/zephyr
```

---

## 🔨 빌드 방법

### QEMU 시뮬레이션

```bash
cd zephyr_app
west build -b qemu_cortex_m3
west build -t run
```

### 실제 하드웨어

```bash
cd zephyr_app

# weact STM32G431CBU6
west build -b weact_stm32g431_core

# Nucleo-G431RB
west build -b nucleo_g431rb

# 클린 빌드
west build -b weact_stm32g431_core --pristine
```

---

## 📤 플래시 방법

### ST-Link 연결

1. ST-Link V2/V3를 USB에 연결
2. STM32G431CBU6 보드의 SWDIO, SWCLK, GND, 3.3V 연결
3. 플래시 실행:

```bash
west flash
```

### Docker에서 플래시

```bash
# 컨테이너 실행 (USB 권한 포함)
docker run -it --rm --privileged -v /dev/bus/usb:/dev/bus/usb space-balloon-ver2:latest

# 컨테이너 안에서
./build.sh
./flash.sh
```

---

## 📊 텔레메트리 출력

### 프레임 구조 (148바이트)

```
┌──────────────────────────────────────────────────────┐
│ [0-1]   0xA5 0x5A (Magic)                            │
│ [2]     0x01 (Version)                               │
│ [3]     0x02 (MsgType: Sensor Snapshot)              │
│ [4-5]   Payload Length (132)                         │
│ [6-7]   Sequence Number                              │
│ [8-11]  Timestamp (ms)                               │
│ [12-143] Payload (132 bytes)                         │
│ [144-145] CRC16                                       │
└──────────────────────────────────────────────────────┘
```

### 전송 사양

- **UART3**: 115200 baud, 8N1
- **주기**: 50Hz (20ms)
- **크기**: 148 bytes × 50 = 7,400 bytes/sec

---

## 🧪 테스트

### QEMU 시뮬레이션 테스트

```bash
west build -b qemu_cortex_m3
west build -t run
```

출력 예:
```
[00:00:01.200] TELEMETRY FRAME: seq=41, uptime=1230ms, flags=0x0009, press=101311Pa
[00:00:01.230] TELEMETRY FRAME: seq=42, uptime=1260ms, flags=0x0009, press=101311Pa
```

### SITL 단위 테스트

```bash
cd SITL/test
mkdir build && cd build
cmake ..
make
./test_fdir_runner
```

---

## 📦 센서 구성

| 센서 | 모델 | 버스 | 주소 |
|------|------|------|------|
| IMU | LSM6DSV16X | I2C1 | 0x6B |
| 자기계 | MLX90393 | I2C1 | 0x0C |
| 방사선 | GDK101 | I2C1 | 0x18 |
| 기압 | MS5611 | I2C3 | 0x77 |
| 온습도 | SHT31-D | I2C3 | 0x44 |
| CO2 | CM1107N | I2C3 | 0x31 |
| 열전대 | MCP9600 | I2C3 | 0x60 |
| 오존 | SEN0321 | I2C3 | 0x70 |
| GPS | XA1110 | UART1 | - |
| 미세먼지 | PMS3003 | UART2 | - |
| 온도 | DS18B20 | 1-Wire | - |

---

## ✅ 포팅 진행률

- [x] 기본 Zephyr 프로젝트 구조
- [x] FDIR 모듈 포팅
- [x] 칼만 필터 포팅
- [x] PID 제어기 포팅
- [x] 텔레메트리 포팅 (UART3 148바이트)
- [x] 센서 드라이버 (Zephyr 내장 + 직접 구현)
- [x] QEMU 시뮬레이션 테스트
- [x] Docker 빌드 환경
- [ ] 실제 하드웨어 테스트

### 센서 드라이버 상태

| 센서 | 모델 | 드라이버 | 상태 |
|------|------|---------|------|
| IMU | LSM6DSV16X | Zephyr 내장 | ✅ |
| 기압 | MS5611 | ms5607 (호환) | ✅ |
| 온습도 | SHT31-D | shtcx (호환) | ✅ |
| 열전대 | MCP9600 | Zephyr 내장 | ✅ |
| 온도 | DS18B20 | Zephyr 내장 | ✅ |
| GPS | XA1110 | gnss-nmea | ✅ |
| 미세먼지 | PMS3003 | pms7003 (호환) | ✅ |
| 자기계 | MLX90393 | 직접 구현 | ✅ |
| 방사선 | GDK101 | 직접 구현 | ✅ |
| CO2 | CM1107N | 직접 구현 | ✅ |
| 오존 | SEN0321 | 직접 구현 | ✅ |

---

## 📝 라이선스

MIT License

---

## 👤 작성자

Hyeonsu Park (MINI-UNIQ-STAR)
