#!/usr/bin/env bash
for arg in "$@"; do
    if [[ "$arg" == "-specs=nosys.specs" ]]; then
        exec /usr/bin/arm-none-eabi-gcc "$@"
    fi
done
exec /usr/bin/arm-none-eabi-gcc -specs=nosys.specs "$@"
