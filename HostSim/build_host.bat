@echo off
gcc -DHOST_TEST_MODE -DUNIT_TEST ^
    -Imock_inc ^
    -I../Core/Inc ^
    -I../Core/Drivers/lsm6dsv16x ^
    -I../Core/Drivers/ms5611 ^
    -I../Core/Drivers/sht31 ^
    -I../Core/Drivers/mlx90393 ^
    -I../Core/Drivers/cm1107n ^
    -I../Core/Drivers/ds18b20 ^
    -I../Core/Drivers/gdk101 ^
    -I../Core/Drivers/mcp9600 ^
    -I../Core/Drivers/pms3003 ^
    -I../Core/Drivers/sen0321 ^
    -I../Core/Drivers/xa1110 ^
    ../Src/test_host.c ^
    ../Core/Src/app.c ^
    ../Core/Src/kalman.c ^
    ../Core/Src/pid.c ^
    ../Core/Src/telemetry.c ^
    ../Core/Src/fdir.c ^
    ../Core/Src/xcp.c ^
    mock_sensors.c ^
    mock_hal.c ^
    -o test_host.exe
if %ERRORLEVEL% NEQ 0 (
    echo Build Failed!
    exit /b %ERRORLEVEL%
)
echo Build Success!
test_host.exe
