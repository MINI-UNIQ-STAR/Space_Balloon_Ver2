# OpenOCD runner example (adjust interface/target as needed)
board_runner_args(openocd "--cmd-pre-init" "source [find interface/stlink.cfg]" "--cmd-pre-init" "source [find target/stm32g4x.cfg]")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
