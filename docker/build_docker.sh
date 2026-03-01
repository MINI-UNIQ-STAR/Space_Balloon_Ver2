#!/bin/bash
# Space Balloon Ver2 - Unified Docker Environment Setup

PROJECT_ROOT=$(pwd)
DOCKER_DIR="${PROJECT_ROOT}/docker"

echo "================================================================"
echo " Space Balloon Ver2 - Unified Docker Environment Setup"
echo "================================================================"

# Check if docker is installed
if ! [ -x "$(command -v docker)" ]; then
  echo "Error: docker is not installed." >&2
  exit 1
fi

# Build image
echo "Building Docker image 'space-balloon-dev'..."
docker build -t space-balloon-dev:latest -f docker/Dockerfile .

if [ $? -eq 0 ]; then
  echo "----------------------------------------------------------------"
  echo " Build Success!"
  echo " To start the environment, run:"
  echo "   docker-compose up -d  (for background)"
  echo "   docker exec -it space-balloon-dev bash (for interactive shell)"
  echo " Or use the following command for quick access:"
  echo "   docker run -it --rm --privileged -v \$(pwd):/workspace space-balloon-dev bash"
  echo "----------------------------------------------------------------"
else
  echo "Error: Docker build failed."
  exit 1
fi
