# bsp.c Board Support Package 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | BSP (HAL 래퍼) |
| **코드 라인** | 408줄 |
| **역할** | 하드웨어 추상화 레이어 |
| **최종 업데이트** | 2026-01-18 |

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
| **버스 복구** | ⭐⭐⭐⭐⭐ | 9-클럭 펄스 I2C 복구 로직 |
| **전원 시퀀스** | ⭐⭐⭐⭐⭐ | 모든 센서(11종) 리셋 해제 및 초기화 지연 적용 |

## Mock 동작

```c
// LSM6DSV16X WHO_AM_I Mock
if (Reg == 0x0F) pData[0] = 0x70;
```

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**HAL 레이어를 완벽하게 래핑하여 상위 레이어에 일관된 하드웨어 액세스 인터페이스를 제공함. 특히 11개 모든 센서의 전원 시퀀스(LSM, SHT, GDK 포함) 정합성을 확보하였으며, I2C 버스 복구 로직과 Mock 지원이 우수함.**
