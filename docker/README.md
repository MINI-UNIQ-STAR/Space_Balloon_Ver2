# Unified Docker Development Environment

This directory contains the unified Docker environment for **Space Balloon Ver2**. It provides a reproducible environment for STM32 bare-metal development, Zephyr RTOS development, Renode simulations, and static analysis.

## Contents

- **Zephyr SDK (v0.16.8)**
- **ARM GNU Toolchain** (gcc-arm-none-eabi)
- **Renode (v1.15.3)**
- **Cppcheck (v2.13.0)**
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

## Requirements
- Docker Engine 20.10+
- Docker Compose v2.0+
- USB access (for flashing) require `--privileged` flag
