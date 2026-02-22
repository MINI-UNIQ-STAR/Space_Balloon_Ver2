# Space Balloon Ver2 - Zephyr RTOS Build Environment
# Multi-platform Docker image for building and flashing

FROM ubuntu:22.04

LABEL maintainer="Hyeonsu Park"
LABEL description="Zephyr RTOS build environment for Space Balloon Ver2"
LABEL version="1.0"

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    # Build tools
    build-essential \
    cmake \
    ninja-build \
    gcc \
    g++ \
    # Python
    python3 \
    python3-pip \
    python3-venv \
    # Version control
    git \
    # Utilities
    wget \
    curl \
    xz-utils \
    file \
    # USB/Serial for flashing
    usbutils \
    udev \
    # Clean up
    && rm -rf /var/lib/apt/lists/*

# Install west (Zephyr meta-tool)
RUN pip3 install west

# Set working directory
WORKDIR /workspace

# Download and install Zephyr SDK 0.16.8
RUN wget -q https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64.tar.xz \
    && tar xf zephyr-sdk-0.16.8_linux-x86_64.tar.xz \
    && mv zephyr-sdk-0.16.8 /opt/zephyr-sdk \
    && rm zephyr-sdk-0.16.8_linux-x86_64.tar.xz \
    && /opt/zephyr-sdk/setup.sh

# Initialize Zephyr project
RUN west init /workspace/zephyrproject
WORKDIR /workspace/zephyrproject
RUN west update
RUN west zephyr-export

# Install Python requirements
RUN pip3 install -r /workspace/zephyrproject/zephyr/scripts/requirements.txt

# Set environment variables
ENV ZEPHYR_BASE=/workspace/zephyrproject/zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

# Copy project source
COPY zephyr_app /workspace/space_balloon_ver2

# Build script
COPY build.sh /workspace/build.sh
RUN chmod +x /workspace/build.sh

# Flash script
COPY flash.sh /workspace/flash.sh
RUN chmod +x /workspace/flash.sh

WORKDIR /workspace

# Default command
CMD ["/bin/bash"]
