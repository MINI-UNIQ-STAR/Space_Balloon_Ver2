#!/bin/bash
source /home/uniqstar-sw/esp/esp-idf/export.sh
qemu-system-xtensa -nographic -machine esp32 -m 4M -drive file=/home/uniqstar-sw/Space_Balloon_Ver2/telemetry_rx_idf/telemetry_rx/build/flash_image.bin,if=mtd,format=raw -serial file:esp_qemu.log -serial tcp:127.0.0.1:4455,server,nowait -serial null < /dev/null &
