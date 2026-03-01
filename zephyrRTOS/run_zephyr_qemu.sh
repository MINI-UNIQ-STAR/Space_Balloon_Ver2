#!/bin/bash
qemu-system-arm -machine lm3s6965evb -nographic -kernel /home/uniqstar-sw/zephyrproject/zephyr/build/zephyr/zephyr.elf -serial file:zephyr_qemu.log -serial tcp:127.0.0.1:4455 < /dev/null &
