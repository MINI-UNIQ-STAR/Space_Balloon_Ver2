cmake_minimum_required(VERSION 3.10)
project(UnitTests C)

set(CMAKE_C_STANDARD 11)

# Link math library
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lm")

# Definitions for Host Test Mode
add_definitions(-DUNIT_TEST)

# Include Directories
set(DRIVER_INCLUDES
    ../../../stm32cube/Core/Drivers/lsm6dsv16x
    ../../../stm32cube/Core/Drivers/cm1107n
    ../../../stm32cube/Core/Drivers/ds18b20
    ../../../stm32cube/Core/Drivers/gdk101
    ../../../stm32cube/Core/Drivers/mcp9600
    ../../../stm32cube/Core/Drivers/minmea
    ../../../stm32cube/Core/Drivers/mlx90393
    ../../../stm32cube/Core/Drivers/ms5611
    ../../../stm32cube/Core/Drivers/pms3003
    ../../../stm32cube/Core/Drivers/sen0321
    ../../../stm32cube/Core/Drivers/sht31
    ../../../stm32cube/Core/Drivers/xa1110
)

include_directories(
    unity_minimal
    ../../../stm32cube/Core/Inc
    test_fdir
    ${DRIVER_INCLUDES}
)

# Unity Source
set(UNITY_SRC unity_minimal/unity.c)

# --- Test FDIR ---
add_executable(test_fdir_runner
    test_fdir/test_main.c
    test_fdir/mock_dependencies.c
    ../../../stm32cube/Core/Src/fdir.c
    ${UNITY_SRC}
)

target_compile_definitions(test_fdir_runner PRIVATE HOST_TEST_MODE)

target_include_directories(test_fdir_runner PRIVATE
    unity_minimal
    test_fdir
    ../HostSim/mock_inc
    ../../../stm32cube/Core/Inc
)

# --- Test PID ---
add_executable(test_pid_runner
    test_pid/test_main.c
    ../../../stm32cube/Core/Src/pid.c
    ${UNITY_SRC}
)

target_include_directories(test_pid_runner PRIVATE
    unity_minimal
    test_pid
    ../../../stm32cube/Core/Inc
    ../HostSim/mock_inc
)

# --- Test Kalman ---
add_executable(test_kalman_runner
    test_kalman/test_main.c
    ../../../stm32cube/Core/Src/kalman.c
    ${UNITY_SRC}
)

target_include_directories(test_kalman_runner PRIVATE
    unity_minimal
    test_kalman
    ../../../stm32cube/Core/Inc
    ../HostSim/mock_inc
)

# --- Test Drivers ---
add_executable(test_drivers_runner
    test_drivers/test_main.c
    test_drivers/mock_hal.c
    test_drivers/sensors_copy.c
    ../../../stm32cube/Core/Src/fdir.c
    test_drivers/mock_lsm6dsv16x.c
    ../../../stm32cube/Core/Drivers/mlx90393/mlx90393_driver.c
    ../../../stm32cube/Core/Drivers/xa1110/xa1110_driver.c
    ../../../stm32cube/Core/Drivers/minmea/minmea.c
    ../../../stm32cube/Core/Drivers/gdk101/gdk101_driver.c
    ../../../stm32cube/Core/Drivers/ms5611/ms5611_driver.c
    ../../../stm32cube/Core/Drivers/sht31/sht31_driver.c
    ../../../stm32cube/Core/Drivers/pms3003/pms3003_driver.c
    ../../../stm32cube/Core/Drivers/sen0321/sen0321_driver.c
    ../../../stm32cube/Core/Drivers/mcp9600/mcp9600_driver.c
    ../../../stm32cube/Core/Drivers/cm1107n/cm1107n_driver.c
    ../../../stm32cube/Core/Drivers/ds18b20/ds18b20_driver.c
    ../../../stm32cube/Core/Drivers/ds18b20/ds18b20.c
    ../../../stm32cube/Core/Drivers/ds18b20/ownet.c
    ../../../stm32cube/Core/Drivers/ds18b20/owhal_mock.c
    ../../../stm32cube/Core/Drivers/ds18b20/crcutil.c
    ${UNITY_SRC}
)

target_compile_definitions(test_drivers_runner PRIVATE HOST_TEST_MODE)

target_include_directories(test_drivers_runner PRIVATE
    unity_minimal
    test_drivers
    ../../../stm32cube/Core/Inc
    ${DRIVER_INCLUDES}
)

target_link_libraries(test_drivers_runner PRIVATE m)

# --- Test Integration ---
add_executable(test_integration_runner
    test_integration/test_mission.c
    ../../../stm32cube/Core/Src/app.c
    ../../../stm32cube/Core/Src/actuators.c
    ../../../stm32cube/Core/Src/fdir.c
    ../../../stm32cube/Core/Src/kalman.c
    ../../../stm32cube/Core/Src/pid.c
    ../../../stm32cube/Core/Src/telemetry.c
    ../../../stm32cube/Core/Src/xcp.c
    ../HostSim/mock_sensors.c
    ../HostSim/mock_hal.c
    ../../../stm32cube/Core/Drivers/minmea/minmea.c
    ${UNITY_SRC}
)

target_compile_definitions(test_integration_runner PRIVATE HOST_TEST_MODE)

target_link_libraries(test_integration_runner PRIVATE m)

target_include_directories(test_integration_runner PRIVATE
    unity_minimal
    test_integration
    ../HostSim/mock_inc
    ../../../stm32cube/Core/Inc
    ${DRIVER_INCLUDES}
)
