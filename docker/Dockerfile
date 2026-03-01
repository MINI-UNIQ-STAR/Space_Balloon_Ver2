# Space Balloon Ver2 - Unified Development Environment
# Includes Zephyr RTOS, STM32 Bare-metal, Renode, ESP-IDF, and Static Analysis tools.

FROM ubuntu:22.04

LABEL maintainer="Hyeonsu Park"
LABEL description="Unified Development Environment for Space Balloon Ver2"
LABEL version="1.1"

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Update and install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    gcc \
    g++ \
    gperf \
    python3 \
    python3-pip \
    python3-venv \
    python3-setuptools \
    python3-tk \
    python3-wheel \
    python3-dev \
    git \
    wget \
    curl \
    xz-utils \
    file \
    make \
    libncurses5-dev \
    libncursesw5-dev \
    libssl-dev \
    usbutils \
    udev \
    cppcheck \
    openocd \
    minicom \
    # QEMU for ARM and ESP32
    qemu-system-arm \
    qemu-system-xtensa \
    qemu-user \
    # ESP-IDF dependencies
    flex \
    bison \
    libffi-dev \
    dfu-util \
    libusb-1.0-0 \
    # Renode dependencies
    libgtk2.0-0 \
    libcanberra-gtk-module \
    libglib2.0-0 \
    gnupg \
    && rm -rf /var/lib/apt/lists/*

# Install ESP-IDF v5.5.1
RUN git clone --recursive -b v5.5.1 https://github.com/espressif/esp-idf.git /opt/esp-idf \
    && cd /opt/esp-idf \
    && ./install.sh esp32 \
    && rm -rf /tmp/*

# Install ARM GNU Toolchain (standard version for STM32Cube)
RUN apt-get update && apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi \
    && rm -rf /var/lib/apt/lists/*

# Install West and Zephyr dependencies
RUN pip3 install --no-cache-dir west==1.2.0

# Install Zephyr SDK 0.16.8 (Minimal core for ARM)
RUN wget -q https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64.tar.xz \
    && tar xf zephyr-sdk-0.16.8_linux-x86_64.tar.xz \
    && mv zephyr-sdk-0.16.8 /opt/zephyr-sdk \
    && rm zephyr-sdk-0.16.8_linux-x86_64.tar.xz \
    && /opt/zephyr-sdk/setup.sh -t arm-zephyr-eabi -c

# Install Renode (Portable version)
RUN wget -q https://github.com/renode/renode/releases/download/v1.15.3/renode-1.15.3.linux-portable.tar.gz \
    && tar xf renode-1.15.3.linux-portable.tar.gz \
    && mv renode-1.15.3_portable /opt/renode \
    && rm renode-1.15.3.linux-portable.tar.gz
ENV PATH="/opt/renode:${PATH}"

# Set working directory
WORKDIR /workspace

# Set Environment Variables
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV IDF_PATH=/opt/esp-idf
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Setup ESP-IDF environment alias
RUN echo "alias get_idf='. /opt/esp-idf/export.sh'" >> /root/.bashrc

# Default command
CMD ["/bin/bash"]
