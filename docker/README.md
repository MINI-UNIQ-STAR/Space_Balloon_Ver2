# Unified Docker Development Environment

This directory contains the unified Docker environment for **Space Balloon Ver2**. It provides a reproducible environment for STM32 bare-metal development, Zephyr RTOS development, Renode simulations, and static analysis.

## Contents

- **Zephyr SDK (v0.16.8)**
- **ARM GNU Toolchain** (gcc-arm-none-eabi)
- **Renode (v1.15.3)**
- **Cppcheck (v2.13.0)**
- **ESP-IDF (v5.5.1)**
- **QEMU** (ARM & Xtensa)
- **Python 3.10+** (with all necessary testing packages)

## Quick Start

### 1. Build the Image

Run the build script from the project root:

```bash
chmod +x docker/build_docker.sh
./docker/build_docker.sh
```

### 2. Enter the Environment

You can use `docker run` directly or `docker-compose`:

**Using Docker Run:**
```bash
docker run -it --rm --privileged -v $(pwd):/workspace space-balloon-dev:latest bash
```

**Using Docker Compose:**
```bash
docker-compose up -d
docker exec -it space-balloon-dev bash
```

## Common Tasks Inside Container

### Build STM32 Bare-metal
```bash
cd /workspace/stm32cube
mkdir build && cd build
cmake .. && make
```

### Flash STM32 Bare-metal
Ensure the ST-Link is connected via USB.
```bash
cd /workspace/stm32cube
openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program build/stm32_spaceballoon.elf verify reset exit"
```

### Build Zephyr App
```bash
cd /workspace/zephyrRTOS/zephyr_app
west build -b weact_stm32g431_core .
```

### Run Static Analysis
```bash
bash /home/uniqstar-sw/embedded-lab/gates/gate_runner.sh /workspace stm32 --only 2
```

### Run Renode HIL Tests
```bash
cd /workspace/tests/RENODE_TEST\(HIL\)
python3 run_fdir_tests.py
```

### Build ESP32 Telemetry RX (ESP-IDF)
```bash
# 1. Activate ESP-IDF environment
get_idf

# 2. Build project
cd /workspace/telemetry/telemetry_rx_idf/telemetry_rx
idf.py build
```

### Flash ESP32 Telemetry RX
Ensure the ESP32 is connected via USB (e.g., `/dev/ttyUSB0`).
```bash
cd /workspace/telemetry/telemetry_rx_idf/telemetry_rx
idf.py -p /dev/ttyUSB0 flash monitor
```

## Requirements
- Docker Engine 20.10+
- Docker Compose v2.0+
- Linux/macOS: USB access (for flashing) requires `--privileged` flag and `/dev` mount.

---

## 🪟 Windows (Docker Desktop) Users

Windows의 Docker Desktop 환경에서는 기본적으로 **USB 장치(ST-Link, ESP32 등) 패스스루가 지원되지 않습니다.** 따라서 Windows 환경에서 펌웨어를 장치에 올리려면 다음 두 가지 방법 중 하나를 선택해야 합니다.

### 방법 1: Docker에서 빌드, Windows에서 플래싱 (가장 쉬움)
컨테이너 환경에서 빌드만 수행하고, 생성된 바이너리 파일(`.elf`, `.bin`)을 Windows에 설치된 툴(STM32CubeProgrammer 등)을 이용해 굽는 방식입니다.
- 코드는 호스트와 공유(`:/workspace`)되므로 컨테이너에서 빌드하면 Windows 폴더에도 결과물이 즉시 나타납니다.

### 방법 2: WSL2와 usbipd-win 사용 (고급)
Windows의 USB 포트를 WSL2 내부로 강제 연결하여 Docker 컨테이너가 인식하게 하는 방법입니다.

1. Windows에 [usbipd-win](https://github.com/dorssel/usbipd-win) 설치
2. 관리자 권한 PowerShell에서 장치 연결:
   ```powershell
   # USB 장치 목록 확인
   usbipd list
   
   # ST-Link/ESP32 연결 (예: BUSID가 2-1인 경우)
   usbipd bind --busid 2-1
   usbipd attach --wsl --busid 2-1
   ```
3. 이후 `docker-compose up -d` 등 Linux와 동일한 방식으로 컨테이너에 접속하여 `openocd`나 `idf.py flash`를 사용 가능.
