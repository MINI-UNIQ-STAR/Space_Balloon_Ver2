#!/bin/bash
# Flash script for Space Balloon Ver2 Zephyr project

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Space Balloon Ver2 - Flash${NC}"
echo -e "${GREEN}========================================${NC}"

# Environment
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
export ZEPHYR_BASE=/workspace/zephyrproject/zephyr

# Activate virtual environment
source /workspace/zephyrproject/.venv/bin/activate 2>/dev/null || true

cd /workspace/space_balloon_ver2

# Check if build exists
if [ ! -f build/zephyr/zephyr.elf ]; then
    echo -e "${RED}Error: No build found. Run ./build.sh first.${NC}"
    exit 1
fi

# Detect connected ST-Link
echo -e "${YELLOW}Detecting ST-Link devices...${NC}"
lsusb | grep -i "st-link\|stlink" || echo "No ST-Link found"

# Flash using west
echo -e "${GREEN}Flashing...${NC}"
west flash

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Flash Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
