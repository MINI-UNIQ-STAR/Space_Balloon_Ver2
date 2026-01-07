@echo off
"C:\Users\hyuns\.gemini\tools\mingw\mingw64\bin\gcc.exe" -DHOST_TEST_MODE -DUNIT_TEST ^
    -I. ^
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
    -I../Core/Drivers/minmea ^
    ../Core/Src/test_host.c ^
    ../Core/Src/app.c ^
    ../Core/Src/kalman.c ^
    ../Core/Src/pid.c ^
    ../Core/Src/telemetry.c ^
    ../Core/Src/fdir.c ^
    ../Core/Src/actuators.c ^
    ../Core/Src/xcp.c ^
    ../Core/Src/bsp.c ^
    ../Core/Src/sensors.c ^
    ../Core/Drivers/lsm6dsv16x/lsm6dsv16x_reg.c ^
    ../Core/Drivers/sht31/sht31_driver.c ^
    ../Core/Drivers/mlx90393/mlx90393_driver.c ^
    ../Core/Drivers/cm1107n/cm1107n_driver.c ^
    ../Core/Drivers/ds18b20/ds18b20.c ^
    ../Core/Drivers/ds18b20/ds18b20_driver.c ^
    ../Core/Drivers/ds18b20/owlink.c ^
    ../Core/Drivers/ds18b20/ownet.c ^
    ../Core/Drivers/ds18b20/crcutil.c ^
    ../Core/Drivers/gdk101/gdk101_driver.c ^
    ../Core/Drivers/mcp9600/mcp9600_driver.c ^
    ../Core/Drivers/pms3003/pms3003_driver.c ^
    ../Core/Drivers/sen0321/sen0321_driver.c ^
    ../Core/Drivers/xa1110/xa1110_driver.c ^
    ../Core/Drivers/ms5611/ms5611_driver.c ^
    ../Core/Drivers/minmea/minmea.c ^
    mock_hal.c ^
    -o test_host.exe > build_log.txt 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Build Failed!
    exit /b %ERRORLEVEL%
)
echo Build Success!
test_host.exe
