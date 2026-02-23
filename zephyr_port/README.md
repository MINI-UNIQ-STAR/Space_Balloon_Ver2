# Zephyr Port Skeleton for Space_Balloon_Ver2

This directory contains an initial Zephyr milestone skeleton that is intentionally isolated from the existing STM32Cube project.

## Included Files
- `app/src/main.c`: minimal hello-style app with periodic heartbeat log
- `app/prj.conf`: baseline console and logging config
- `app/CMakeLists.txt`: Zephyr app build entry
- `PORTING_PLAN.md`: phased migration plan

## Build (example: qemu_x86)
From a Zephyr workspace shell with environment initialized:

```bash
west build -b qemu_x86 /home/uniqstar-sw/Space_Balloon_Ver2/zephyr_port/app
```

## Run
```bash
west build -t run
```

You should see startup and periodic log messages every second.

## Custom board scaffold (STM32G431CBU6)
A starter board definition is included at:
`zephyr_port/boards/arm/stm32g431cbu6/`

Build with BOARD_ROOT:
```bash
cd ~/zephyrproject/zephyr
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=~/zephyrproject/zephyr-sdk-0.16.8
west build -p always -b stm32g431cbu6 /home/uniqstar-sw/Space_Balloon_Ver2/zephyr_port/app -- -DBOARD_ROOT=/home/uniqstar-sw/Space_Balloon_Ver2/zephyr_port
```

> Note: pin mapping/clock tree/flash layout still need tuning for your real PCB.
