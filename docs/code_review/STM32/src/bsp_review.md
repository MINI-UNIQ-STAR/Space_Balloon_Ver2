# bsp.c Board Support Package 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | BSP (HAL 래퍼) |
| **코드 라인** | 283줄 |
| **역할** | 하드웨어 추상화 레이어 |

## 구조

| 그룹 | 함수 | 설명 |
|------|------|------|
| **Init** | `BSP_Init()`, `BSP_Sensor_PowerOn()` | 초기화 |
| **I2C1** | `WriteReg`, `ReadReg`, `Write`, `Read`, `Recovery` | Downside 버스 |
| **I2C3** | `WriteReg`, `ReadReg`, `Recovery` | Upside 버스 |
| **UART** | `Write`, `Read` | 시리얼 통신 |
| **System** | `GetTick`, `Delay` | 시스템 |
| **ADC** | `Read_Battery_mV` | 배터리 전압 |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **추상화** | ⭐⭐⭐⭐⭐ | HAL 완전 래핑 |
| **Mock 지원** | ⭐⭐⭐⭐⭐ | UNIT_TEST 분기 |
| **센서 주소** | ⭐⭐⭐⭐⭐ | bsp.h에 정의 |

## Mock 동작

```c
// LSM6DSV16X WHO_AM_I Mock
if (Reg == 0x0F) pData[0] = 0x70;
```

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**이식성 핵심 레이어.**
