# SITL (Software-In-The-Loop) Simulation Guide

## Overview

The HostSim SITL environment runs the STM32 balloon firmware on your host PC (Windows/Linux) for testing and validation without hardware. It uses real radiosonde flight data from RS41 to simulate a realistic balloon flight scenario.

## Components

### 1. Flight Data Source
- **File**: `flight_data.h` (auto-generated from `simulation_reference_data/V4630075.json`)
- **Generator**: `convert_flight_data.py`
- **Data Points**: 65 unique frames from real RS41 radiosonde flight
- **Altitude Range**: 5174m - 5631m
- **Content**: GPS coordinates, altitude, velocity, temperature, battery voltage, satellite count, heading

### 2. Mock Sensor System
- **File**: `mock_sensors.c`
- **Features**:
  - Flight data playback with 10x interpolation for smooth simulation
  - ISA atmospheric model for pressure calculation
  - Altitude-dependent temperature, humidity, and radiation simulation
  - Realistic IMU, magnetometer, and air quality data generation
  - Fault injection capability for FDIR testing

### 3. Mock HAL Layer
- **Files**: `mock_hal.c`, `mock_inc/mock_hal.h`
- **Purpose**: Provides stub implementations of STM32 HAL functions
- **Simulates**: I2C, UART, TIM, ADC, GPIO operations

### 4. Build System
- **CMake**: `CMakeLists.txt` (cross-platform)
- **Batch Script**: `build_host.bat` (Windows quick build)
- **Compiler**: MinGW GCC (Windows) or native GCC (Linux)
- **Output**: `test_host.exe` (Windows) or `test_host` (Linux)

## Building the SITL

### Prerequisites
- **Windows**: MinGW-w64 GCC (typically at `C:\Users\<user>\.gemini\tools\mingw\mingw64\bin\gcc.exe`)
- **Linux**: GCC (`sudo apt install gcc cmake`)

### Method 1: Quick Build (Windows)
```bash
cd HostSim
./build_host.bat
```
This will:
1. Compile all source files
2. Link into `test_host.exe`
3. Run the simulation automatically if build succeeds

### Method 2: CMake Build (Cross-platform)
```bash
cd HostSim
mkdir -p build
cd build
cmake ..
make
./test_host  # or test_host.exe on Windows
```

### Build Output
- **Success**: Creates `test_host.exe` (~1.5 MB)
- **Logs**: Build warnings/errors saved to `build_log.txt`
- **Common Warnings**: Implicit HAL function declarations (expected for mock layer)

## Running the SITL

### Basic Execution
```bash
cd HostSim
./test_host.exe  # Windows
./test_host      # Linux
```

### Expected Output
```
Starting Host Test (App Layer)...
[Mock] Sensors Initialized - RS41 Flight Data Mode
[Mock] Loaded 65 flight data points
[Mock] Altitude range: 5174m - 5631m
[Pass] App Initialized.
Running Loop...
Seq: 0, Fix: 1, Bat: 2800 mV
Seq: 10, Fix: 1, Bat: 2800 mV
Seq: 20, Fix: 1, Bat: 2900 mV
Seq: 30, Fix: 1, Bat: 2900 mV
Seq: 40, Fix: 1, Bat: 2800 mV
Test Finished.
```

### Execution Parameters
- **Duration**: 50 loop iterations (simulates ~1 second at 50Hz)
- **Status Updates**: Printed every 10 iterations
- **Telemetry**: Sequence number, GPS fix status, battery voltage

## Flight Data Generation

To regenerate `flight_data.h` from updated radiosonde data:

```bash
cd HostSim
python convert_flight_data.py
```

**Requirements**: Python 3.x, JSON file at `../simulation_reference_data/V4630075.json`

**Output**: Regenerates `flight_data.h` with 65 flight data points

## Simulation Features

### 1. Realistic Flight Profile
- **GPS Trajectory**: Real coordinates from RS41 radiosonde
- **Interpolation**: 10x sub-steps between data points for smooth motion
- **Altitude**: 5.2-5.6 km altitude range
- **Velocity**: Vertical and horizontal velocity profiles from flight data

### 2. Environmental Simulation
| Parameter | Model | Range |
|-----------|-------|-------|
| **Temperature** | RS41 data or ISA (-6.5°C/1000m) | -50°C to -26°C |
| **Pressure** | ISA barometric formula | 50-60 kPa |
| **Humidity** | Altitude-dependent (decreases with altitude) | 5-50% RH |
| **Radiation** | Increases with altitude | 0.1-0.6 µSv/h |

### 3. Sensor Data
- **GPS**: 9 satellites, fix type 1, realistic lat/lon/alt
- **IMU**: Acceleration with ascent dynamics, small gyro oscillations
- **Magnetometer**: Heading-based magnetic field (25 µT horizontal, 45 µT vertical)
- **Air Quality**: CO2 400ppm, PM2.5 15µg/m³, Ozone 30ppb
- **Battery**: Real voltage profile from RS41 (2.7-2.9V range)

### 4. FDIR Testing Support
Mock sensors support fault injection for testing failure detection:
- `FAULT_BARO_RANGE`: Out-of-range pressure values
- `FAULT_GPS_JUMP`: Sudden altitude jumps
- `FAULT_BARO_TIMEOUT`: Sensor timeout simulation
- `FAULT_GPS_TIMEOUT`: GPS loss simulation

*Note: Fault injection API exists but requires manual code modification to activate*

## What Gets Tested

The SITL validates:
- ✅ **App Layer Logic**: Initialization, main loop, control flow
- ✅ **Telemetry System**: Frame generation, sequence numbers
- ✅ **Sensor Integration**: Data flow from mock sensors to telemetry
- ✅ **PID Controllers**: Heater control algorithms
- ✅ **Kalman Filter**: Altitude fusion
- ✅ **FDIR System**: Failure detection and recovery (with fault injection)
- ✅ **XCP Protocol**: Calibration parameter handling

### NOT Tested (Requires Hardware)
- ❌ I2C/UART/SPI communication timing
- ❌ Interrupt latency and priorities
- ❌ DMA transfers
- ❌ ADC sampling accuracy
- ❌ GPIO electrical characteristics
- ❌ LoRa radio transmission/reception

## Interpreting Results

### Success Indicators
- `[Pass] App Initialized.` - App layer started successfully
- Increasing sequence numbers - Main loop executing
- `Fix: 1` - GPS simulation working
- Battery voltage cycling between 2700-2900 mV - Realistic battery model

### Failure Indicators
- Compilation errors in `build_log.txt`
- Missing mock function implementations
- Assertion failures (if added)
- Infinite loops or hangs

## Extending the SITL

### Adding Manual Test Scenarios
Use `Sensors_SetMockData()` in `test_host.c`:

```c
// Test low voltage scenario
Sensors_SetMockData(5500.0f, -30.0f, 55000.0f);
for (int i = 0; i < 100; i++) {
    App_Loop();
}
```

### Adding Fault Injection Tests
Modify `test_host.c` to inject faults:

```c
// Inject GPS altitude jump at frame 20 for 50 frames
MockSensors_InjectFault(FAULT_GPS_JUMP, 20, 50);
```

### Output Data Analysis
Currently, SITL prints to stdout. To capture data for analysis:

```bash
./test_host.exe > simulation_output.txt
```

To add CSV output for plotting, modify `test_host.c` to write telemetry frames to file.

## File Structure

```
HostSim/
├── README.md                    # This file
├── CMakeLists.txt               # CMake build configuration
├── build_host.bat               # Windows quick build script
├── convert_flight_data.py       # Flight data converter
├── flight_data.h                # Generated flight data (65 points)
├── mock_sensors.c               # Sensor simulation implementation
├── mock_hal.c                   # HAL stub implementations
├── mock_inc/
│   ├── mock_hal.h               # HAL type definitions
│   └── stm32g4xx_hal.h          # STM32 HAL header stub
├── build/                       # CMake build directory
│   └── test_host.exe            # SITL executable (after build)
├── build_log.txt                # Latest build log
└── test_host.exe                # SITL executable (root location)
```

## Troubleshooting

### Build Fails with "gcc not found"
- **Windows**: Update `build_host.bat` with correct MinGW path
- **Linux**: Install GCC: `sudo apt install gcc`

### Missing Header Errors
- Check `CMakeLists.txt` includes all necessary driver directories
- Verify all `#include` paths are correct

### Linker Errors (Undefined Reference)
- Add missing source files to `APP_SOURCES` in `CMakeLists.txt` or `build_host.bat`
- Implement missing mock functions in `mock_hal.c` or `mock_sensors.c`

### Simulation Hangs
- Check for infinite loops in app logic
- Verify mock functions don't block indefinitely

### Incorrect Sensor Values
- Regenerate `flight_data.h`: `python convert_flight_data.py`
- Check interpolation logic in `mock_sensors.c`

## Version History

- **Rev 1.0** (2026-01-09): Initial SITL documentation with RS41 flight data support

---

For hardware integration testing (HITL), see main project documentation.
