# Zephyr Porting Plan (Milestone v0)

## Goal
Establish a minimal, buildable Zephyr application baseline for `Space_Balloon_Ver2` before migrating STM32Cube-based firmware modules.

## Milestone Scope
- Create standalone Zephyr app skeleton under `zephyr_port/app`
- Verify baseline boot and periodic logging on a reference board (`qemu_x86`)
- Keep existing STM32Cube project untouched

## Phased Migration Plan
1. Baseline Setup
- Confirm Zephyr SDK/toolchain setup and west workspace readiness
- Keep current Cube firmware as source of truth

2. Board and BSP Alignment
- Select target Zephyr board for STM32G431 (or custom board definition)
- Map clock/UART/GPIO baseline in devicetree/Kconfig

3. Driver and HAL Abstraction
- Identify hardware-facing modules in current firmware
- Replace direct HAL calls with Zephyr subsystems (GPIO, I2C, SPI, UART, ADC, PWM)

4. RTOS Integration
- Convert periodic loops and timers to Zephyr threads/workqueues/timers
- Introduce message queues/ring buffers for sensor and telemetry pipelines

5. Sensor and Telemetry Bring-up
- Port sensor interfaces with deterministic sampling intervals
- Port telemetry path and validate frame formatting/checksums

6. Test and Validation
- Add unit/integration checks for critical modules
- Execute long-run stability and watchdog/recovery scenarios

## Exit Criteria for Next Milestone
- App builds and runs on selected Zephyr board target
- UART logging available and stable
- Initial hardware abstraction layer mapping document completed
