# Renode 시뮬레이션 구현 노트

## 개요

이 문서는 STM32G431CBU6 기반 라디오존데의 Renode 시뮬레이션 구현에 대한 기술적 세부사항을 설명합니다.

## 아키텍처

### 플랫폼 계층

```
┌─────────────────────────────────────┐
│   Firmware (Bare-metal)             │
│   - app.c, sensors.c, etc.          │
└─────────────────────────────────────┘
              ↓ HAL API
┌─────────────────────────────────────┐
│   STM32 HAL Driver                  │
│   - I2C, UART, ADC, Timer, etc.     │
└─────────────────────────────────────┘
              ↓ MMIO
┌─────────────────────────────────────┐
│   Renode Platform (stm32g431.repl)  │
│   - Memory map                      │
│   - Peripheral definitions          │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│   Sensor Models (Python/C#)         │
│   - I2C peripherals                 │
│   - UART devices                    │
└─────────────────────────────────────┘
```

## 구현된 컴포넌트

### 1. 플랫폼 정의 (stm32g431.repl)

**기반**: `platforms/cpus/stm32g4.repl`

**커스터마이징**:
- 메모리 크기 조정 (Flash: 128KB, SRAM: 32KB)
- I2C1, I2C3 추가
- UART1, UART2, UART3 설정
- ADC1 (배터리 전압 모니터링)
- 타이머 (TIM1, TIM8, TIM16)
- DMA 채널
- GPIO 포트 (A, B, C, D, F)

**주요 메모리 맵**:
```
0x08000000 - 0x0801FFFF  Flash (128KB)
0x20000000 - 0x20007FFF  SRAM (32KB)
0x40005400 - 0x400057FF  I2C1
0x40007800 - 0x40007BFF  I2C3
0x40013800 - 0x40013BFF  USART1
0x40004400 - 0x400047FF  USART2
0x40004800 - 0x40004BFF  USART3
```

### 2. I2C 센서 모델

#### LSM6DSV16X (lsm6dsv16x.py)

**구현 내용**:
- WHO_AM_I 레지스터 (0x0F = 0x70)
- 가속도계 출력 레지스터 (0x28-0x2D)
- 자이로스코프 출력 레지스터 (0x22-0x27)
- 시뮬레이션: 중력 + 정현파 움직임

**데이터 스케일**:
- 가속도: ±2g 범위, 16-bit
- 자이로: ±250 dps 범위, 16-bit

**코드 예시**:
```python
def GetAccelData(self, axis):
    if axis == 2:  # Z-axis
        accel_g = 1.0 + noise  # 1g (gravity)
    raw = int(accel_g * 32768 / 2.0)
    return raw & 0xFFFF
```

#### MS5611 (ms5611.py)

**구현 내용**:
- PROM 읽기 (캘리브레이션 계수)
- D1 변환 (압력 ADC)
- D2 변환 (온도 ADC)
- 고도 기반 기압 계산

**기압 공식**:
```python
P = P0 * exp(-altitude / H)
# P0 = 101325 Pa (해수면)
# H = 8400m (스케일 높이)
```

**시뮬레이션 동작**:
- 고도가 시간에 따라 정현파로 변화 (1000m ± 500m)
- 기압이 고도에 반비례

#### SHT31 (sht31.py)

**구현 내용**:
- Single-shot 측정 명령 (0x2C 0x06)
- 온도/습도 데이터 + CRC-8 체크섬
- Soft reset (0x30 0xA2)

**데이터 변환**:
```python
# Temperature: -45 ~ 130°C
temp_raw = (T + 45) * 65535 / 175

# Humidity: 0 ~ 100%
rh_raw = RH * 65535 / 100
```

**CRC-8 계산**: polynomial 0x31 사용

#### MLX90393 (mlx90393.py)

**구현 내용**:
- Start burst mode (0x3E)
- Read measurement (0x4E)
- 3축 자기장 데이터 (X, Y, Z)

**시뮬레이션**:
- 지구 자기장 (~50 uT)
- Z축 중심 회전 시뮬레이션

### 3. UART 센서

#### GPS XA1110 (gps_xa1110.cs)

**상태**: 부분 구현 (C# 스켈레톤 코드)

**계획된 기능**:
- NMEA 문장 생성 (GGA, RMC, GSA)
- 실시간 UTC 시간
- 위도/경도 변화 시뮬레이션
- 고도 상승 패턴

**NMEA 예시**:
```
$GPGGA,123456.00,3505.5788,N,12659.9322,E,1,09,1.0,100.0,M,0.0,M,,*XX
$GPRMC,123456.00,A,3505.5788,N,12659.9322,E,0.0,0.0,110126,,*XX
```

## Renode 특화 고려사항

### IronPython 제약

Renode는 .NET 기반 IronPython을 사용:

**사용 가능**:
- `math` 모듈
- 기본 Python 문법
- `memcpy` 대신 배열 슬라이싱

**사용 불가**:
- NumPy, SciPy 등 CPython 확장
- 일부 표준 라이브러리 (threading, multiprocessing)

### 타이밍 시뮬레이션

Renode는 가상 시간 사용:

```resc
# 양자(quantum) 설정 - 작을수록 정밀, 느림
emulation SetGlobalQuantum "0.0001"

# 특정 시간 실행
emulation RunFor "5.0"  # 5초
```

### 메모리 접근

직접 메모리 읽기/쓰기:

```
(monitor) sysbus ReadDoubleWord 0x40005400  # I2C1 CR1
(monitor) sysbus WriteDoubleWord 0x40005400 0x1  # Enable I2C1
```

## 디버깅 전략

### 1. I2C 통신 추적

```resc
logLevel -1 sysbus.i2c1
```

출력 예시:
```
[I2C1] Write to 0x6B: [0x0F]
[I2C1] Read from 0x6B: [0x70] (WHO_AM_I)
```

### 2. Python 센서 로그

센서 모델 내부:

```python
self.DebugLog("Accel X={0:.2f} Y={1:.2f} Z={2:.2f}".format(x, y, z))
```

### 3. GDB 연동

Renode에서:
```
machine StartGdbServer 3333
```

GDB에서:
```bash
arm-none-eabi-gdb build/Debug/stm32_spaceballoon.elf
(gdb) target remote :3333
(gdb) b Sensors_Init
(gdb) c
```

## 알려진 제한사항

### 1. 1-Wire 프로토콜

- DS18B20 센서 미구현
- Renode에 기본 1-Wire 지원 없음
- **해결책**: GPIO 비트뱅잉 시뮬레이션 필요 (복잡)

### 2. UART 센서 타이밍

- GPS NMEA 전송 속도가 실제와 다를 수 있음
- **해결책**: Renode 타이머 훅 사용

### 3. DMA 전송

- 일부 DMA 시나리오에서 타이밍 이슈 가능
- **해결책**: 인터럽트 기반 대안 테스트

### 4. ADC 노이즈

- 실제 ADC 노이즈 시뮬레이션 없음
- **해결책**: Python으로 노이즈 추가

## 확장 가이드

### 새 I2C 센서 추가

1. **센서 모델 작성** (`sensors/new_sensor.py`):

```python
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Core import EmptyRegisterPeripheralBase

class NewSensor(EmptyRegisterPeripheralBase, II2CPeripheral):
    def __init__(self):
        EmptyRegisterPeripheralBase.__init__(self)
        self.registers = {}

    def Write(self, data):
        # I2C 쓰기 처리
        pass

    def Read(self, count):
        # I2C 읽기 처리
        return []

    def FinishTransmission(self):
        pass

    def Reset(self):
        pass
```

2. **simulation.resc 수정**:

```python
python
"""
execfile('renode/sensors/new_sensor.py')
machine.SystemBus.GetI2CPeripheralByName('i2c1').Register(NewSensor(), 0xAA)
"""
```

### 새 UART 장치 추가

C#으로 구현 (Python보다 UART 처리 용이):

```csharp
using Antmicro.Renode.Peripherals.UART;

public class NewUARTDevice : IExternal, IUART
{
    public event Action<byte> CharReceived;

    public void WriteChar(byte value) { }
    public void Reset() { }

    public Bits StopBits => Bits.One;
    public Parity ParityBit => Parity.None;
    public uint BaudRate => 9600;
}
```

## 성능 최적화

### 시뮬레이션 속도

- **병렬화**: 여러 머신 동시 실행 가능
- **프로파일링**: Renode 내장 프로파일러 사용
- **선택적 로깅**: 필요한 부분만 로그 활성화

### 메모리 사용

- **센서 데이터 캐싱**: 불필요한 재계산 방지
- **로그 버퍼 제한**: 장시간 실행 시 메모리 누수 방지

## 베스트 프랙티스

1. **점진적 통합**: 한 번에 하나의 센서씩 추가 및 테스트
2. **단위 테스트**: 각 센서 모델을 독립적으로 검증
3. **실제 데이터 비교**: 가능하면 실제 하드웨어 출력과 대조
4. **문서화**: 각 센서의 시뮬레이션 가정 명시
5. **버전 관리**: Renode 버전 명시 (호환성)

## 참고 자료

### Renode 문서
- [공식 문서](https://renode.readthedocs.io/)
- [Python 페리페럴](https://renode.readthedocs.io/en/latest/advanced/writing-peripherals.html)
- [플랫폼 정의](https://renode.readthedocs.io/en/latest/advanced/platform-description-format.html)

### STM32G4 자료
- [참조 매뉴얼 RM0440](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [데이터시트 STM32G431](https://www.st.com/resource/en/datasheet/stm32g431cb.pdf)

### 센서 데이터시트
- LSM6DSV16X: ST MEMS 센서
- MS5611: TE Connectivity
- SHT31: Sensirion
- MLX90393: Melexis

## 버전 히스토리

- **v1.0** (2026-01-11): 초기 구현
  - STM32G431 플랫폼 정의
  - I2C 센서 4개 (LSM6DSV16X, MS5611, SHT31, MLX90393)
  - 기본 시뮬레이션 스크립트
