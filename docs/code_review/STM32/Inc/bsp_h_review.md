# bsp.h Board Support Package 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 103줄 |
| **역할** | 하드웨어 추상화 API |

## I2C 주소 정의

| 버스 | 센서 | 주소 |
|------|------|------|
| **I2C1** | LSM6DSV16X | 0x6B |
| | MLX90393 | 0x0C |
| | GDK101 | 0x18 |
| **I2C3** | SHT31 | 0x44 |
| | MS5611 | 0x77 |
| | CM1107N | 0x31 |
| | MCP9600 | 0x60 |
| | SEN0321 | 0x70 |

## API 그룹

| 그룹 | 함수 |
|------|------|
| **Init** | `BSP_Init`, `BSP_Sensor_PowerOn` |
| **I2C1** | `WriteReg`, `ReadReg`, `Write`, `Read` |
| **I2C3** | `WriteReg`, `ReadReg` |
| **UART** | `Write`, `Read` |
| **System** | `GetTick`, `Delay` |
| **ADC** | `Read_Battery_mV` |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **Doxygen** | ⭐⭐⭐⭐⭐ | 모든 함수 문서화 |
| **주소 중앙화** | ⭐⭐⭐⭐⭐ | 매크로 정의 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
