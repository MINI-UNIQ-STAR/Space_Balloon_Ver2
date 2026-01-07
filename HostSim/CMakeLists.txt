cmake_minimum_required(VERSION 3.10)
project(HostSim C)

set(CMAKE_C_STANDARD 11)

# Definitions for Host Mode
add_definitions(-DHOST_TEST_MODE)
add_definitions(-DUNIT_TEST)

# Include Directories
include_directories(
    mock_inc
    ../Core/Inc
    ../Core/Drivers/lsm6dsv16x
    ../Core/Drivers/ms5611
    ../Core/Drivers/sht31
    ../Core/Drivers/mlx90393
    ../Core/Drivers/cm1107n
    ../Core/Drivers/ds18b20
    ../Core/Drivers/gdk101
    ../Core/Drivers/mcp9600
    ../Core/Drivers/pms3003
    ../Core/Drivers/sen0321
    ../Core/Drivers/xa1110
    # Add other driver includes if App logic depends on their structs
)

# Source Files (App Logic ONLY, No HAL Driver sources)
set(APP_SOURCES
    ../Core/Src/test_host.c
    ../Core/Src/app.c
    ../Core/Src/kalman.c
    ../Core/Src/pid.c
    ../Core/Src/telemetry.c
    ../Core/Src/fdir.c
    ../Core/Src/xcp.c
    # Use mock_sensors.c instead of Core/Src/sensors.c to avoid driver dependencies
    mock_sensors.c
    mock_hal.c
)

# Add Executable
add_executable(test_host ${APP_SOURCES})
