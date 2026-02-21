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
