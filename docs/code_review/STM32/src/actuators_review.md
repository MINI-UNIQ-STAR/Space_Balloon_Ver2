# actuators.c 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Hardware Abstraction |
| **코드 라인** | 58줄 |
| **역할** | PWM 히터 제어 |

## 구조

| 함수 | 설명 |
|------|------|
| `Actuators_Init()` | TIM3/TIM8 PWM 시작 |
| `Actuators_SetHeater_Battery()` | PA6 히터 Duty |
| `Actuators_SetHeater_Board()` | PC6 히터 Duty |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **입력 검증** | ⭐⭐⭐⭐⭐ | 0~100% 클램핑 |
| **Mock 지원** | ⭐⭐⭐⭐⭐ | UNIT_TEST 분기 |
| **간결성** | ⭐⭐⭐⭐⭐ | 58줄 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**간결한 PWM 래퍼.**
