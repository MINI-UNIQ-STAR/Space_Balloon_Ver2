cmake_minimum_required(VERSION 3.10)
project(HostSim C)

set(CMAKE_C_STANDARD 11)

# Definitions for Host Mode
add_definitions(-DHOST_TEST_MODE)
add_definitions(-DUNIT_TEST)

# Include Directories
include_directories(
    mock_inc
    ../../Core/Inc
    ../../Core/Drivers/lsm6dsv16x
    ../../Core/Drivers/ms5611
    ../../Core/Drivers/sht31
    ../../Core/Drivers/mlx90393
    ../../Core/Drivers/cm1107n
    ../../Core/Drivers/ds18b20
    ../../Core/Drivers/gdk101
    ../../Core/Drivers/mcp9600
    ../../Core/Drivers/pms3003
    ../../Core/Drivers/sen0321
    ../../Core/Drivers/xa1110
    # Add other driver includes if App logic depends on their structs
)

# Source Files (App Logic ONLY, No HAL Driver sources)
set(APP_SOURCES
    ../../Core/Src/test_host.c
    ../../Core/Src/app.c
    ../../Core/Src/actuators.c
    ../../Core/Src/kalman.c
    ../../Core/Src/pid.c
    ../../Core/Src/telemetry.c
    ../../Core/Src/fdir.c
    ../../Core/Src/xcp.c
    ../../Core/Src/bsp.c
    ../../Core/Src/sensors.c
    # Drivers
    ../../Core/Drivers/lsm6dsv16x/lsm6dsv16x_reg.c
    ../../Core/Drivers/sht31/sht31_driver.c
    ../../Core/Drivers/mlx90393/mlx90393_driver.c
    ../../Core/Drivers/cm1107n/cm1107n_driver.c
    ../../Core/Drivers/ds18b20/ds18b20.c
    ../../Core/Drivers/ds18b20/ds18b20_driver.c
    ../../Core/Drivers/ds18b20/owlink.c
    ../../Core/Drivers/ds18b20/ownet.c
    ../../Core/Drivers/ds18b20/crcutil.c
    ../../Core/Drivers/gdk101/gdk101_driver.c
    ../../Core/Drivers/mcp9600/mcp9600_driver.c
    ../../Core/Drivers/pms3003/pms3003_driver.c
    ../../Core/Drivers/sen0321/sen0321_driver.c
    ../../Core/Drivers/xa1110/xa1110_driver.c
    ../../Core/Drivers/ms5611/ms5611_driver.c
    ../../Core/Drivers/minmea/minmea.c
    
    mock_hal.c
)

# Add Executable
add_executable(test_host ${APP_SOURCES})
