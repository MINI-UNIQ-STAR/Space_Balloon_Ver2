# FDIR Test Results

## Test Summary
**Date**: 2026-01-11  
**Total Tests**: 22  
**Passed**: 22  
**Failed**: 0  
**Success Rate**: 100%

## Test Results

### UART/GPIO Tests (Basic Verification)
| ID | Test Name | Result |
|----|-----------|--------|
| U-01 | Firmware Boot Verification | ✅ PASS |
| U-02 | GPIO Port Activity | ✅ PASS |
| U-03 | Timer Interrupt Test | ✅ PASS |

### Sensor Disconnection Tests (I2C/UART)
| ID | Sensor | Protocol | Reset Pin | Result |
|----|--------|----------|-----------|--------|
| S-01 | LSM6DSV16X (IMU) | I2C1 | PB13 | ✅ PASS |
| S-02 | MLX90393 (Magnetometer) | I2C1 | PB14 | ✅ PASS |
| S-03 | GDK101 (Radiation) | I2C1 | PB2 | ✅ PASS |
| S-04 | SEN0321 (Ozone) | I2C3 | PC9 | ✅ PASS |
| S-07 | MS5611 (Barometer) | I2C3 | PA5 | ✅ PASS |
| S-08 | SHT31D (Humidity) | I2C3 | PB11 | ✅ PASS |
| S-09 | MCP9600 (Thermocouple) | I2C3 | PC8 | ✅ PASS |
| S-11 | CM1107N (CO2) | I2C3 | PB0 | ✅ PASS |
| S-05 | XA1110 (GPS) | UART1 | PA9 | ✅ PASS |
| S-06 | PMS3003 (Dust) | UART3 | PB10 | ✅ PASS |

### GPIO Peripheral Tests
| ID | Component | GPIO Pin | Result |
|----|-----------|----------|--------|
| S-10 | DS18B20 (Ext Temp) | PB15 | ✅ PASS |
| H-01 | Kapton Heater | PA6 (PWM) | ✅ PASS |
| H-02 | Minibulb | PC6 (PWM) | ✅ PASS |
| T-01 | UART3 Timing (20ms) | - | ✅ PASS |

### System Peripheral Tests (NEW)
| ID | Feature | Description | Result |
|----|---------|-------------|--------|
| P-01 | PPS Sync | GPS 1Hz Interrupt (PB4) | ✅ PASS |
| P-02 | ADC Battery | Battery Voltage (PA1) | ✅ PASS |
| P-03 | Timer2 | Telemetry Tick | ✅ PASS |
| P-04 | Timer6 | Main Loop Tick | ✅ PASS |
| P-06 | EXTI Interrupts | Sensor INT Pins | ✅ PASS |

## Coverage
- ✅ All 7 I2C sensors FDIR tested
- ✅ All UART sensors FDIR tested
- ✅ All Actuators (Heater, Bulb) verified
- ✅ **PPS & Time Sync verified**
- ✅ **ADC Battery Monitor verified**
- ✅ **System Timers verified**
- ❌ LoRa SPI (External module, not tested hereby user request)
