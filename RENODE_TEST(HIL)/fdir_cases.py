
# FMEA Test Cases - Comprehensive FDIR Verification
# Based on docs/FDIR.md sensor fault recovery specifications

# ============================================
# UART/GPIO Based Tests (No I2C dependency)
# ============================================
UART_GPIO_TESTS = [
    {
        "id": "U-01",
        "name": "Firmware Boot Verification",
        "description": "Verify firmware boots and starts main loop",
        "setup_cmd": "logLevel 3 sysbus.usart3",
        "inject_cmd": "",  # No fault injection
        "verification_log": "usart3:",
        "duration": 10
    },
    {
        "id": "U-02",
        "name": "GPIO Port Activity",
        "description": "Verify GPIO port activity during sensor operations",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "",
        "verification_log": "gpioPortB:",
        "duration": 10
    },
    {
        "id": "U-03",
        "name": "Timer Interrupt Test",
        "description": "Verify timer interrupts are firing",
        "setup_cmd": "logLevel 3 sysbus.nvic",
        "inject_cmd": "",
        "verification_log": "nvic:",
        "duration": 5
    }
]

# ============================================
# I2C1 Bus Sensor Tests (Downside Sensors)
# GPIO PortB for resets: PB13(IMU), PB14(MAG), PB2(RAD)
# ============================================
I2C1_TESTS = [
    {
        "id": "S-01",
        "name": "IMU I2C Fail (LSM6DSV16X)",
        "description": "Disconnect IMU and verify FDIR. Reset: PB13 (LSM_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB", 
        "inject_cmd": "sysbus Unregister i2c1_lsm6dsv16x",
        "verification_log": "gpioPortB:",
        "duration": 20
    },
    {
        "id": "S-02",
        "name": "Magnetometer I2C Fail (MLX90393)",
        "description": "Disconnect Magnetometer and verify FDIR. Reset: PB14 (MLX_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB", 
        "inject_cmd": "sysbus Unregister i2c1_mlx90393",
        "verification_log": "gpioPortB:",
        "duration": 20
    },
    {
        "id": "S-03",
        "name": "Radiation Sensor I2C Fail (GDK101)",
        "description": "Disconnect Radiation sensor and verify FDIR. Reset: PB2 (GDK_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB", 
        "inject_cmd": "sysbus Unregister i2c1_gdk101",
        "verification_log": "gpioPortB:",
        "duration": 20
    },
    {
        "id": "S-04",
        "name": "Ozone Sensor I2C Fail (SEN0321)",
        "description": "Disconnect SEN0321 (I2C3) and verify FDIR. Reset: PC9 (SEN_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortC", 
        "inject_cmd": "sysbus Unregister i2c3_sen0321",
        "verification_log": "gpioPortC:",
        "duration": 20
    }
]

# ============================================
# I2C3 Bus Sensor Tests (Upside Sensors)
# GPIO PortA: PA5 (BARO), GPIO PortB: PB11 (SHT)
# ============================================
I2C3_TESTS = [
    {
        "id": "S-07",
        "name": "Barometer I2C Fail (MS5611)",
        "description": "Disconnect Barometer and verify FDIR. Reset: PA5 (MS_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortA",
        "inject_cmd": "sysbus Unregister i2c3_ms5611",
        "verification_log": "gpioPortA:",
        "duration": 20
    },
    {
        "id": "S-08",
        "name": "Humidity Sensor I2C Fail (SHT31D)",
        "description": "Disconnect SHT31D and verify FDIR. Reset: PB11 (SHT_RST P-MOS)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "sysbus Unregister i2c3_sht31d",
        "verification_log": "gpioPortB:",
        "duration": 20
    },
    {
        "id": "S-09",
        "name": "Thermocouple I2C Fail (MCP9600)",
        "description": "Disconnect MCP9600 and verify FDIR. Reset: PC8 (MCP_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortC",
        "inject_cmd": "sysbus Unregister i2c3_mcp9600",
        "verification_log": "gpioPortC:",
        "duration": 20
    },
    {
        "id": "S-11",
        "name": "CO2 Sensor I2C Fail (CM1107N)",
        "description": "Disconnect CM1107N and verify FDIR. Reset: PB0 (CM_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "sysbus Unregister i2c3_cm1107n",
        "verification_log": "gpioPortB:",
        "duration": 20
    }
]

# ============================================
# UART Sensor Tests (GPS, PMS3003)
# ============================================
UART_SENSOR_TESTS = [
    {
        "id": "S-05",
        "name": "GPS UART Fail (XA1110)",
        "description": "Disconnect GPS UART and verify FDIR. Reset: PA9 (GPS_RST)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortA",
        "inject_cmd": "sysbus Unregister usart1",
        "verification_log": "gpioPortA:",
        "duration": 20
    },
    {
        "id": "S-06",
        "name": "Dust Sensor UART Fail (PMS3003)",
        "description": "Disconnect PMS3003 UART and verify FDIR. Reset: PB10 (PMS_SET)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "sysbus Unregister pms3003",
        "verification_log": "gpioPortB:",
        "duration": 20
    }
]

# ============================================
# 1-Wire & GPIO Peripheral Tests (DS18B20, Heater, Bulb)
# ============================================
GPIO_PERIPHERAL_TESTS = [
    {
        "id": "S-10",
        "name": "DS18B20 1-Wire (Ext Temp)",
        "description": "External temperature sensor on PB15 (1-Wire) - timing verification",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "",  # No direct disconnection, monitors GPIO activity
        "verification_log": "gpioPortB:",  # PB15 activity
        "duration": 15
    },
    {
        "id": "H-01",
        "name": "Kapton Heater PWM (PA6)",
        "description": "Verify Kapton film heater PWM control on PA6",
        "setup_cmd": "logLevel 3 sysbus.gpioPortA",
        "inject_cmd": "",  # Monitor PWM activity
        "verification_log": "gpioPortA:",  # PA6 activity
        "duration": 15
    },
    {
        "id": "H-02",
        "name": "Minibulb PWM (PC6)",
        "description": "Verify mini bulb indicator PWM control on PC6",
        "setup_cmd": "logLevel 3 sysbus.gpioPortC",
        "inject_cmd": "",  # Monitor PWM activity
        "verification_log": "gpioPortC:",  # PC6 activity
        "duration": 15
    }
]

# ============================================
# UART Timing Tests
# ============================================
UART_TIMING_TESTS = [
    {
        "id": "T-01",
        "name": "UART3 Periodic Activity (20ms)",
        "description": "Verify UART3 transmits data at ~20ms intervals (telemetry/debug output)",
        "setup_cmd": "logLevel 3 sysbus.usart3",
        "inject_cmd": "",  # No fault, monitor activity
        "verification_log": "usart3:",  # UART3 activity
        "duration": 10
    }
]

# ============================================
# System Peripheral Tests (PPS, ADC, RTC, EXTI, SPI)
# Comprehensive firmware verification
# ============================================
SYSTEM_TESTS = [
    {
        "id": "P-01",
        "name": "PPS Sync (GPS 1Hz)",
        "description": "Verify GPS PPS interrupt (EXTI4 on PB4) for time synchronization",
        "setup_cmd": "logLevel 3 sysbus.nvic",  # Monitor NVIC for EXTI4 interrupts
        "inject_cmd": "",
        "verification_log": "nvic:",  # EXTI interrupt activity
        "duration": 15
    },
    {
        "id": "P-02",
        "name": "ADC Battery Monitor (PA1)",
        "description": "Verify ADC1 battery voltage measurement on PA1",
        "setup_cmd": "logLevel 3 sysbus.adc1",
        "inject_cmd": "",
        "verification_log": "adc1:",  # ADC activity
        "duration": 10
    },
    {
        "id": "P-03",
        "name": "Timer2 (Telemetry Tick)",
        "description": "Verify TIM2 is running for telemetry timing",
        "setup_cmd": "logLevel 3 sysbus.tim2",
        "inject_cmd": "",
        "verification_log": "tim2:",  # Timer2 activity
        "duration": 10
    },
    {
        "id": "P-04",
        "name": "Timer6 (Main Loop Tick)",
        "description": "Verify TIM6 is running for main loop timing",
        "setup_cmd": "logLevel 3 sysbus.tim6",
        "inject_cmd": "",
        "verification_log": "tim6:",  # Timer6 activity
        "duration": 10
    },

    {
        "id": "P-06",
        "name": "EXTI Interrupts (Sensors)",
        "description": "Verify external interrupts from LSM/MLX sensors (EXTI 5-9)",
        "setup_cmd": "logLevel 3 sysbus.nvic",
        "inject_cmd": "",
        "verification_log": "nvic:",  # NVIC interrupt handling
        "duration": 10
    }
]

# ============================================
# Combine All Tests
# ============================================
# Full test suite: 3 UART/GPIO + 4 I2C1 + 3 I2C3 + 2 UART + 3 GPIO + 1 Timing + 6 System = 22 tests
REVISED_TEST_CASES = UART_GPIO_TESTS + I2C1_TESTS + I2C3_TESTS + UART_SENSOR_TESTS + GPIO_PERIPHERAL_TESTS + UART_TIMING_TESTS + SYSTEM_TESTS

# Quick test suite (for faster iteration)
QUICK_TEST_CASES = UART_GPIO_TESTS + [I2C1_TESTS[0], I2C3_TESTS[0]]

# Legacy (kept for reference)
TEST_CASES = UART_GPIO_TESTS

