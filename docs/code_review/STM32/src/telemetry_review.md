# telemetry.c 텔레메트리 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Communication |
| **코드 라인** | 100줄 |
| **역할** | 데이터 프레임 전송 |

## 구조

| 함수 | 설명 |
|------|------|
| `CRC16_CCITT()` | 체크섬 계산 |
| `Telemetry_PrintFrame()` | 디버그 출력 |
| `Telemetry_Send()` | UART 전송 |

## CRC 알고리즘
```
CRC16-CCITT FALSE
Poly: 0x1021
Init: 0xFFFF
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **CRC** | ⭐⭐⭐⭐⭐ | 표준 구현 |
| **디버그** | ⭐⭐⭐⭐⭐ | 상세 출력 |
| **Mock** | ⭐⭐⭐⭐⭐ | HOST_TEST_MODE |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
