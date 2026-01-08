# main.h 메인 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 120줄 |
| **생성** | STM32CubeMX |
| **역할** | 글로벌 포함, GPIO 정의 |

## HOST_TEST_MODE 지원
```c
#if defined(HOST_TEST_MODE)
#define STM32G4xx_HAL_H  // HAL 억제
#include "mock_hal.h"
#endif
```

## GPIO 핀 정의 (CubeMX 생성)

| 그룹 | 핀 |
|------|------|
| **ADC** | BAT_measure (PA1) |
| **PWM** | Kapton_PWM (PA6), Minibulb_PWM (PC6) |
| **Reset** | MCP_RST, MS_RST, CM1107N_RST, SEN_RST 등 |
| **GPS** | XA1110_RST/Wake/INT/PPS |
| **IMU** | LSM_RST/INT |
| **1-Wire** | DS18B20_Pin (PB15) |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **Mock 지원** | ⭐⭐⭐⭐⭐ | HOST_TEST_MODE |
| **핀 명명** | ⭐⭐⭐⭐⭐ | CubeMX 표준 |
| **포함 관리** | ⭐⭐⭐⭐⭐ | 조건부 HAL |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
