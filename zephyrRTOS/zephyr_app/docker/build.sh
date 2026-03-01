#!/bin/bash
# Build script for Space Balloon Ver2 Zephyr project

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Space Balloon Ver2 - Zephyr Build${NC}"
echo -e "${GREEN}========================================${NC}"

# Environment
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
export ZEPHYR_BASE=/workspace/zephyrproject/zephyr

# Activate virtual environment
source /workspace/zephyrproject/.venv/bin/activate 2>/dev/null || true

cd /workspace/space_balloon_ver2

# Parse arguments
BOARD=${1:-weact_stm32g431_core}
CLEAN=${2:-}

echo -e "${YELLOW}Board: ${BOARD}${NC}"
echo -e "${YELLOW}Clean: ${CLEAN}${NC}"

# Build
if [ "$CLEAN" = "clean" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf build
fi

echo -e "${GREEN}Building...${NC}"
west build -b ${BOARD}

# Show result
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build Complete!${NC}"
echo -e "${GREEN}========================================${NC}"

# Show binary info
if [ -f build/zephyr/zephyr.elf ]; then
    echo -e "${YELLOW}Output:${NC}"
    ls -lh build/zephyr/zephyr.elf
    echo ""
    arm-zephyr-eabi-size build/zephyr/zephyr.elf || true
fi

echo ""
echo -e "${YELLOW}To flash: ./flash.sh${NC}"
