# Space Balloon Ver2 - Docker Build Environment

> [!IMPORTANT]
> **권장 사항**: 이 개별 Docker 환경 대신 프로젝트 루트의 [통합 Docker 환경](../../../docker/README.md) 사용을 권장합니다. 통합 환경은 Zephyr뿐만 아니라 STM32 베어메탈, Renode, 정적 분석 도구를 모두 포함하고 있습니다.

## Quick Start

### Build Docker Image

```bash
# 프로젝트 루트에서 실행
cd Space_Balloon_Ver2
docker build -t space-balloon-ver2:latest -f zephyr_app/docker/Dockerfile .
```

### Run Container

```bash
# Interactive mode
docker run -it --rm space-balloon-ver2:latest

# With USB device for flashing
docker run -it --rm --privileged -v /dev/bus/usb:/dev/bus/usb space-balloon-ver2:latest
```

### Build Firmware

```bash
# Inside container
./build.sh                    # Build for weact STM32G431CBU6
./build.sh qemu_cortex_m3     # Build for QEMU simulation
./build.sh weact_stm32g431_core clean  # Clean build
```

### Flash to Hardware

```bash
# Inside container (ST-Link connected)
./flash.sh
```

## Transfer to Another Computer

### Export Image

```bash
cd Space_Balloon_Ver2/zephyr_app/docker
./export.sh
```

This creates `space-balloon-ver2-docker.tar.gz` (~1GB)

### Import on Another Computer

```bash
# Load image
docker load < space-balloon-ver2-docker.tar.gz

# Run container
docker run -it --rm space-balloon-ver2:latest
```

## Directory Structure

```
Space_Balloon_Ver2/
├── zephyr_app/           # Project source
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── src/
│   ├── include/
│   ├── boards/
│   └── docker/
│       ├── Dockerfile    # Docker image definition
│       ├── build.sh      # Build script
│       ├── flash.sh      # Flash script
│       ├── export.sh     # Export image for transfer
│       └── README.md     # This file
```

## Requirements

- Docker 20.10+
- ST-Link V2/V3 (for flashing)
- USB permissions (for flashing)

## Image Contents

| Component | Version |
|-----------|---------|
| Ubuntu | 22.04 |
| Zephyr SDK | 0.16.8 |
| Python | 3.x |
| ARM Toolchain | Included |

## Image Size

- Base image: ~500MB
- With Zephyr SDK: ~1.5GB
- Total: ~2GB
- Export file: ~1GB (compressed)

## Supported Targets

| Target | Description |
|--------|-------------|
| `qemu_cortex_m3` | QEMU simulation |
| `weact_stm32g431_core` | weact STM32G431CBU6 |
| `nucleo_g431rb` | Nucleo-G431RB |

## Notes

- The image includes full Zephyr SDK and toolchain
- Build artifacts are stored in `/workspace/space_balloon_ver2/build/`
- For flashing, run container with `--privileged` flag
- All 11 sensor drivers are included
