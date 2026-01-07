cmake_minimum_required(VERSION 3.10)
project(UnitTests C)

set(CMAKE_C_STANDARD 11)

# Definitions for Host Test Mode

add_definitions(-DUNIT_TEST)

# Include Directories
set(DRIVER_INCLUDES
    ../Core/Drivers/lsm6dsv16x
    ../Core/Drivers/cm1107n
    ../Core/Drivers/ds18b20
    ../Core/Drivers/gdk101
    ../Core/Drivers/mcp9600
    ../Core/Drivers/minmea
    ../Core/Drivers/mlx90393
    ../Core/Drivers/ms5611
    ../Core/Drivers/pms3003
    ../Core/Drivers/sen0321
    ../Core/Drivers/sht31
    ../Core/Drivers/xa1110
)

include_directories(
    unity_minimal
    ${CMAKE_CURRENT_SOURCE_DIR}/../HostSim/mock_inc # Prioritize Mocks
    ../Core/Inc
    test_fdir # For mock_dependencies.h
    ${DRIVER_INCLUDES}
)

# Unity Source
set(UNITY_SRC unity_minimal/unity.c)

# --- Test FDIR ---
add_executable(test_fdir_runner
    test_fdir/test_main.c
    test_fdir/mock_dependencies.c
    ../Core/Src/fdir.c
    ${UNITY_SRC}
)

target_compile_definitions(test_fdir_runner PRIVATE HOST_TEST_MODE)

target_include_directories(test_fdir_runner PRIVATE
    unity_minimal
    test_fdir
    ../HostSim/mock_inc
    ../Core/Inc
)

# --- Test PID ---
add_executable(test_pid_runner
    test_pid/test_main.c
    ../Core/Src/pid.c
    ${UNITY_SRC}
)

target_include_directories(test_pid_runner PRIVATE
    unity_minimal
    test_pid
    ../Core/Inc
    ../HostSim/mock_inc
)

# --- Test Kalman ---
add_executable(test_kalman_runner
    test_kalman/test_main.c
    ../Core/Src/kalman.c
    ${UNITY_SRC}
)

target_include_directories(test_kalman_runner PRIVATE
    unity_minimal
    test_kalman
    ../Core/Inc
    ../HostSim/mock_inc
)

# --- Test Drivers ---
add_executable(test_drivers_runner
    test_drivers/test_main.c
    test_drivers/mock_hal.c
    test_drivers/sensors_copy.c
    ../Core/Src/fdir.c
    ../Core/Drivers/lsm6dsv16x/lsm6dsv16x_reg.c
    ../Core/Drivers/mlx90393/mlx90393_driver.c
    ../Core/Drivers/xa1110/xa1110_driver.c
    ../Core/Drivers/minmea/minmea.c
    ../Core/Drivers/gdk101/gdk101_driver.c
    ../Core/Drivers/ms5611/ms5611_driver.c
    ../Core/Drivers/sht31/sht31_driver.c
    ../Core/Drivers/pms3003/pms3003_driver.c
    ../Core/Drivers/sen0321/sen0321_driver.c
    ../Core/Drivers/mcp9600/mcp9600_driver.c
    ../Core/Drivers/cm1107n/cm1107n_driver.c
    ../Core/Drivers/ds18b20/ds18b20_driver.c
    ../Core/Drivers/ds18b20/ds18b20.c
    ../Core/Drivers/ds18b20/ownet.c
    ../Core/Drivers/ds18b20/owhal_mock.c
    ../Core/Drivers/ds18b20/crcutil.c
    ${UNITY_SRC}
)

target_compile_definitions(test_drivers_runner PRIVATE HOST_TEST_MODE)

target_include_directories(test_drivers_runner PRIVATE
    unity_minimal
    test_drivers
    ../HostSim/mock_inc
    ../Core/Inc
    ${DRIVER_INCLUDES}
)

# --- Test Integration ---
add_executable(test_integration_runner
    test_integration/test_mission.c
    ../Core/Src/app.c
    ../Core/Src/actuators.c
    ../Core/Src/fdir.c
    ../Core/Src/kalman.c
    ../Core/Src/pid.c
    ../Core/Src/telemetry.c
    ../Core/Src/xcp.c
    ../HostSim/mock_sensors.c
    ../HostSim/mock_hal.c
    ../Core/Drivers/minmea/minmea.c
    ${UNITY_SRC}
)

target_compile_definitions(test_integration_runner PRIVATE HOST_TEST_MODE)

target_include_directories(test_integration_runner PRIVATE
    unity_minimal
    test_integration
    ../HostSim/mock_inc
    ../Core/Inc
    ${DRIVER_INCLUDES}
)
