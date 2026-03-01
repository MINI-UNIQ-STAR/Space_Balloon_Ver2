# Space Balloon Ver2 Board

Space Balloon high-altitude radiosonde based on STM32G431CBU6.

## Hardware

- **MCU**: STM32G431CBU6 (Cortex-M4F, 170MHz, 128KB Flash, 32KB RAM)
- **Sensors**:
  - IMU: LSM6DSV16X (I2C1, 0x6B)
  - Magnetometer: MLX90393 (I2C1, 0x0C)
  - Radiation: GDK101 (I2C1, 0x18)
  - Barometer: MS5611 (I2C3, 0x77)
  - Humidity: SHT31 (I2C3, 0x44)
  - CO2: CM1107N (I2C3, 0x31)
  - Thermocouple: MCP9600 (I2C3, 0x60)
  - Ozone: SEN0321 (I2C3, 0x70)
  - GPS: XA1110 (UART1)
  - Dust: PMS3003 (UART2)
  - Temp: DS18B20 (1-Wire)
- **Actuators**:
  - Kapton heater (PWM)
  - Minibulb indicator (PWM)

## Pin Mapping

| Function | Pin | Port |
|----------|-----|------|
| I2C1_SCL | PA15 | GPIO |
| I2C1_SDA | PA16 | GPIO |
| I2C3_SCL | PA7 | GPIO |
| I2C3_SDA | PB6 | GPIO |
| UART1_TX (GPS) | PA9 | GPIO |
| UART1_RX (GPS) | PA10 | GPIO |
| UART2_TX (Dust) | PA2 | GPIO |
| UART2_RX (Dust) | PA3 | GPIO |
| UART3_TX (Telemetry) | PB10 | GPIO |
| UART3_RX (Telemetry) | PB11 | GPIO |
| Kapton_PWM | PA6 | GPIO |
| Minibulb_PWM | PC6 | GPIO |
| Battery ADC | PA1 | ADC |
| DS18B20 | PB15 | GPIO |

## Building

```bash
west build -b space_balloon_ver2
```

## Flashing

```bash
west flash
```
