# FMEA Test Cases - Enhanced FDIR Verification v2
# Based on docs/FDIR.md sensor fault recovery specifications
#
# v2 Improvements:
#   - Telemetry-based verification (status_flags bits)
#   - FDIR state machine validation (HEALTHY -> WARNING -> RECOVERY -> FAILURE)
#   - Recovery count tracking
#   - Actual sensor behavior verification

# FDIR status_flags bit positions (telemetry.h)
FLAG_SYS_OK = 0
FLAG_GPS_WARN = 1
FLAG_BARO_WARN = 2
FLAG_IMU_WARN = 3
FLAG_TEMP_WARN = 4
FLAG_HEATER_ACTIVE = 5
FLAG_LOW_BATTERY = 6
FLAG_FDIR_RECOVERY = 7
FLAG_ALT_JUMP = 8
FLAG_RANGE_ERROR = 9

# Expected FDIR bit mappings for each sensor
SENSOR_FLAG_MAP = {
    "IMU": FLAG_IMU_WARN,
    "MAG": FLAG_IMU_WARN,      # Shares IMU_WARN (I2C1 bus)
    "GDK": FLAG_IMU_WARN,      # Shares IMU_WARN (I2C1 bus)
    "GPS": FLAG_GPS_WARN,
    "BARO": FLAG_BARO_WARN,
    "SHT": FLAG_TEMP_WARN,
    "MCP": FLAG_TEMP_WARN,
    "CM": FLAG_TEMP_WARN,      # CO2
    "SEN": FLAG_TEMP_WARN,     # Ozone
    "PMS": FLAG_TEMP_WARN,     # Dust
}

# ============================================
# BASIC SYSTEM TESTS
# ============================================
UART_GPIO_TESTS = [
    {
        "id": "U-01",
        "name": "Firmware Boot Verification",
        "description": "Verify firmware boots and starts main loop",
        "setup_cmd": "logLevel 3 sysbus.usart3",
        "inject_cmd": "",  # No fault injection
        "verification": {
            "type": "telemetry",
            "expect_frames": 10,  # At least 10 frames in 10s at 50Hz
            "min_uptime_ms": 5000,
        },
        "duration": 10
    },
    {
        "id": "U-02",
        "name": "GPIO Port Activity",
        "description": "Verify GPIO port activity during sensor operations",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "gpioPortB:",
        },
        "duration": 10
    },
    {
        "id": "U-03",
        "name": "Timer Interrupt Test",
        "description": "Verify timer interrupts are firing",
        "setup_cmd": "logLevel 3 sysbus.nvic",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "nvic:",
        },
        "duration": 5
    }
]

# ============================================
# I2C1 Bus Sensor Tests - Enhanced
# ============================================
I2C1_TESTS = [
    {
        "id": "S-01",
        "name": "IMU I2C Fail (LSM6DSV16X)",
        "description": "Disconnect IMU and verify FDIR sets IMU_WARN flag",
        "setup_cmd": "logLevel 0 sysbus.usart3",  # Enable telemetry analyzer
        "inject_cmd": "sysbus Unregister i2c1_lsm6dsv16x",
        "inject_delay_ms": 2000,  # Let system stabilize first
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_IMU_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,  # Should trigger within 3s timeout + recovery
        },
        "duration": 25
    },
    {
        "id": "S-02",
        "name": "Magnetometer I2C Fail (MLX90393)",
        "description": "Disconnect Magnetometer and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c1_mlx90393",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_IMU_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    },
    {
        "id": "S-03",
        "name": "Radiation Sensor I2C Fail (GDK101)",
        "description": "Disconnect Radiation sensor and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c1_gdk101",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_IMU_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    },
    {
        "id": "S-04",
        "name": "Ozone Sensor I2C Fail (SEN0321)",
        "description": "Disconnect SEN0321 and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c3_sen0321",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_TEMP_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    }
]

# ============================================
# I2C3 Bus Sensor Tests - Enhanced
# ============================================
I2C3_TESTS = [
    {
        "id": "S-07",
        "name": "Barometer I2C Fail (MS5611)",
        "description": "Disconnect Barometer and verify FDIR sets BARO_WARN flag",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c3_ms5611",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_BARO_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    },
    {
        "id": "S-08",
        "name": "Humidity Sensor I2C Fail (SHT31D)",
        "description": "Disconnect SHT31D and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c3_sht31d",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_TEMP_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    },
    {
        "id": "S-09",
        "name": "Thermocouple I2C Fail (MCP9600)",
        "description": "Disconnect MCP9600 and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c3_mcp9600",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_TEMP_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    },
    {
        "id": "S-11",
        "name": "CO2 Sensor I2C Fail (CM1107N)",
        "description": "Disconnect CM1107N and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c3_cm1107n",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_TEMP_WARN,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    }
]

# ============================================
# UART Sensor Tests - Enhanced
# ============================================
UART_SENSOR_TESTS = [
    {
        "id": "S-05",
        "name": "GPS UART Fail (XA1110)",
        "description": "Disconnect GPS UART and verify FDIR sets GPS_WARN flag",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister usart1",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_GPS_WARN,
            "expect_state": "SET",
            "timeout_ms": 6000,  # GPS has 5s timeout
        },
        "duration": 25
    },
    {
        "id": "S-06",
        "name": "Dust Sensor UART Fail (PMS3003)",
        "description": "Disconnect PMS3003 UART and verify FDIR",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister pms3003",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_TEMP_WARN,
            "expect_state": "SET",
            "timeout_ms": 6000,  # 5s timeout
        },
        "duration": 25
    }
]

# ============================================
# GPIO Peripheral Tests
# ============================================
GPIO_PERIPHERAL_TESTS = [
    {
        "id": "S-10",
        "name": "DS18B20 1-Wire (Ext Temp)",
        "description": "External temperature sensor on PB15 (1-Wire)",
        "setup_cmd": "logLevel 3 sysbus.gpioPortB",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "gpioPortB:",
        },
        "duration": 15
    },
    {
        "id": "H-01",
        "name": "Kapton Heater PWM (PA6)",
        "description": "Verify Kapton film heater PWM control",
        "setup_cmd": "logLevel 3 sysbus.gpioPortA",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "gpioPortA:",
        },
        "duration": 15
    },
    {
        "id": "H-02",
        "name": "Minibulb PWM (PC6)",
        "description": "Verify mini bulb indicator PWM control",
        "setup_cmd": "logLevel 3 sysbus.gpioPortC",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "gpioPortC:",
        },
        "duration": 15
    }
]

# ============================================
# UART Timing Tests
# ============================================
UART_TIMING_TESTS = [
    {
        "id": "T-01",
        "name": "UART3 Telemetry Periodic (50Hz)",
        "description": "Verify UART3 transmits telemetry at ~50Hz (20ms intervals)",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "",
        "verification": {
            "type": "telemetry",
            "expect_frames": 40,  # 40 frames in 10s at 50Hz
            "check_timing": True,  # Verify frame intervals
        },
        "duration": 10
    }
]

# ============================================
# System Peripheral Tests
# ============================================
SYSTEM_TESTS = [
    {
        "id": "P-01",
        "name": "PPS Sync (GPS 1Hz)",
        "description": "Verify GPS PPS interrupt for time synchronization",
        "setup_cmd": "logLevel 3 sysbus.nvic",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "nvic:",
        },
        "duration": 15
    },
    {
        "id": "P-02",
        "name": "ADC Battery Monitor (PA1)",
        "description": "Verify ADC1 battery voltage measurement",
        "setup_cmd": "logLevel 3 sysbus.adc1",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "adc1:",
        },
        "duration": 10
    },
    {
        "id": "P-03",
        "name": "Timer2 (Telemetry Tick)",
        "description": "Verify TIM2 is running for telemetry timing",
        "setup_cmd": "logLevel 3 sysbus.tim2",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "tim2:",
        },
        "duration": 10
    },
    {
        "id": "P-04",
        "name": "Timer6 (Main Loop Tick)",
        "description": "Verify TIM6 is running for main loop timing",
        "setup_cmd": "logLevel 3 sysbus.tim6",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "tim6:",
        },
        "duration": 10
    },
    {
        "id": "P-06",
        "name": "EXTI Interrupts (Sensors)",
        "description": "Verify external interrupts from sensors",
        "setup_cmd": "logLevel 3 sysbus.nvic",
        "inject_cmd": "",
        "verification": {
            "type": "log",
            "pattern": "nvic:",
        },
        "duration": 10
    }
]

# ============================================
# FDIR Recovery Tests (New in v2)
# ============================================
FDIR_RECOVERY_TESTS = [
    {
        "id": "R-01",
        "name": "IMU Recovery After Reconnect",
        "description": "Verify IMU_WARN clears when sensor reconnects after fault",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c1_lsm6dsv16x",
        "reconnect_cmd": "sysbus.i2c1 Register @sensors/lsm6dsv16x.py:LSM6DSV16X() 0x6B",
        "inject_delay_ms": 2000,
        "reconnect_delay_ms": 8000,  # After fault, wait, then reconnect
        "verification": {
            "type": "fdir_sequence",
            "expect_flag": FLAG_IMU_WARN,
            "expect_sequence": ["SET", "CLEARED"],
            "timeout_ms": 15000,
        },
        "duration": 30
    },
    {
        "id": "R-02",
        "name": "FDIR_RECOVERY Flag During Recovery",
        "description": "Verify FDIR_RECOVERY flag is set during recovery attempts",
        "setup_cmd": "logLevel 0 sysbus.usart3",
        "inject_cmd": "sysbus Unregister i2c3_ms5611",
        "inject_delay_ms": 2000,
        "verification": {
            "type": "fdir",
            "expect_flag": FLAG_FDIR_RECOVERY,
            "expect_state": "SET",
            "timeout_ms": 5000,
        },
        "duration": 25
    }
]

# ============================================
# Combine All Tests
# ============================================
REVISED_TEST_CASES = (
    UART_GPIO_TESTS + 
    I2C1_TESTS + 
    I2C3_TESTS + 
    UART_SENSOR_TESTS + 
    GPIO_PERIPHERAL_TESTS + 
    UART_TIMING_TESTS + 
    SYSTEM_TESTS +
    FDIR_RECOVERY_TESTS
)

# Quick test suite for fast iteration
QUICK_TEST_CASES = UART_GPIO_TESTS + [I2C1_TESTS[0], I2C3_TESTS[0], FDIR_RECOVERY_TESTS[0]]

# Legacy compatibility
TEST_CASES = UART_GPIO_TESTS
