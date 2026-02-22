#!/bin/bash
# Export Docker image for transfer to another computer

set -e

echo "========================================"
echo "Exporting Space Balloon Ver2 Docker Image"
echo "========================================"

# Image name
IMAGE_NAME="space-balloon-ver2"
IMAGE_TAG="latest"
OUTPUT_FILE="space-balloon-ver2-docker.tar.gz"

# Build image if not exists
if ! docker image inspect ${IMAGE_NAME}:${IMAGE_TAG} &>/dev/null; then
    echo "Building Docker image..."
    docker build -t ${IMAGE_NAME}:${IMAGE_TAG} .
fi

# Export image
echo "Exporting Docker image to ${OUTPUT_FILE}..."
docker save ${IMAGE_NAME}:${IMAGE_TAG} | gzip > ${OUTPUT_FILE}

# Show result
echo ""
echo "========================================"
echo "Export Complete!"
echo "========================================"
echo ""
ls -lh ${OUTPUT_FILE}
echo ""
echo "Transfer this file to another computer and run:"
echo "  docker load < ${OUTPUT_FILE}"
echo ""
