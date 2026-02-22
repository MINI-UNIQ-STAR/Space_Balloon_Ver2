# Space Balloon Ver2 - Docker Build Environment

## Quick Start

### Build Docker Image

```bash
cd docker
docker build -t space-balloon-ver2:latest .
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
cd docker
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
docker/
├── Dockerfile      # Docker image definition
├── build.sh        # Build script
├── flash.sh        # Flash script
├── export.sh       # Export image for transfer
└── README.md       # This file

zephyr_app/         # Project source (copied into image)
├── CMakeLists.txt
├── prj.conf
├── src/
└── include/
```

## Requirements

- Docker 20.10+
- ST-Link V2/V3 (for flashing)
- USB permissions (for flashing)

## Image Size

- Base image: ~500MB
- With Zephyr SDK: ~1.5GB
- Total: ~2GB
- Export file: ~1GB (compressed)

## Notes

- The image includes full Zephyr SDK and toolchain
- Build artifacts are stored in `/workspace/space_balloon_ver2/build/`
- For flashing, run container with `--privileged` flag
