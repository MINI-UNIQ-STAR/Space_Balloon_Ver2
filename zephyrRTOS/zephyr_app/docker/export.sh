#!/bin/bash
# Export Docker image for transfer to another computer

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Export Space Balloon Ver2 Docker Image${NC}"
echo -e "${GREEN}========================================${NC}"

# Image name
IMAGE_NAME="space-balloon-ver2"
IMAGE_TAG="latest"
OUTPUT_FILE="space-balloon-ver2-docker.tar.gz"

# Check if image exists
if ! docker image inspect ${IMAGE_NAME}:${IMAGE_TAG} &>/dev/null; then
    echo -e "${YELLOW}Docker image not found. Building...${NC}"
    
    # Get script directory
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
    
    cd "${PROJECT_ROOT}"
    docker build -t ${IMAGE_NAME}:${IMAGE_TAG} -f zephyr_app/docker/Dockerfile .
fi

# Export image
echo -e "${YELLOW}Exporting Docker image to ${OUTPUT_FILE}...${NC}"
docker save ${IMAGE_NAME}:${IMAGE_TAG} | gzip > ${OUTPUT_FILE}

# Show result
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Export Complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
ls -lh ${OUTPUT_FILE}
echo ""
echo "Transfer this file to another computer and run:"
echo "  docker load < ${OUTPUT_FILE}"
echo ""
