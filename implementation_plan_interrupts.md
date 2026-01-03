# Implementation Plan: Sensor Interrupt Integration

## 1. Objective
Integrate hardware interrupt handling for the LSM6DSV16x (IMU) and MLX90393 (Magnetometer) sensors into the existing firmware architecture.

## 2. Current Status
- **Hardware**:
  - LSM6DSV16x INT connected to **PB6** (EXTI6).
  - MLX90393 INT connected to **PB7** (EXTI7).
- **Firmware**:
  - `main.c`: NVIC for `EXTI9_5_IRQn` is enabled.
  - `stm32g4xx_it.c`: `EXTI9_5_IRQHandler` calls `HAL_GPIO_EXTI_IRQHandler` for PB6 and PB7.
  - `exti_dispatch.c`: `HAL_GPIO_EXTI_Callback` has placeholders for PB6 and PB7.
  - **Drivers**:
    - `lsm6dsv16x`: Driver exists but lacks interrupt handling functions.
    - `mlx90393`: Driver files (`.c`/`.h`) are **missing** in the current codebase.

## 3. Implementation Steps

### Step 1: Create MLX90393 Driver Skeleton
Since the MLX90393 driver is missing, we need to create the basic files.
- **File**: `Core/Inc/drivers/mlx90393.h`
  - Define basic configuration structs (if needed).
  - Declare `mlx90393_init()` (placeholder).
  - Declare `mlx90393_exti_callback(uint16_t pin)`.
- **File**: `Core/Src/drivers/mlx90393.c`
  - Implement empty/placeholder functions.

### Step 2: Update LSM6DSV16x Driver
Add an interrupt callback function to the existing driver.
- **File**: `Core/Inc/drivers/lsm6dsv16x.h`
  - Declare `void lsm6dsv16x_exti_callback(uint16_t pin);`.
- **File**: `Core/Src/drivers/lsm6dsv16x.c`
  - Implement the function (initially just a placeholder or a flag setter).

### Step 3: Integrate into EXTI Dispatcher
Connect the low-level EXTI callback to the driver functions.
- **File**: `Core/Src/drivers/exti_dispatch.c`
  - Include `drivers/lsm6dsv16x.h` and `drivers/mlx90393.h`.
  - Replace `TODO` comments with calls to `lsm6dsv16x_exti_callback(GPIO_Pin)` and `mlx90393_exti_callback(GPIO_Pin)`.

### Step 4: Build & Verify
- Run `pio run` to ensure no linker errors (especially for the new MLX driver).

## 4. Future Considerations
- **Service Layer**: Once the drivers capture the interrupts (e.g., setting a `data_ready` flag), the corresponding services (`imu_service`, `aux_sensors_service`) will need to poll these flags or be notified to read data.
