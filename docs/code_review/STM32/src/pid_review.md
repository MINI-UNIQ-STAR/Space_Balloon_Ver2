# pid.c PID 제어기 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Algorithm |
| **코드 라인** | 93줄 |
| **역할** | 히터 온도 제어 |
| **최종 업데이트** | 2026-01-18 |

## 구조

| 함수 | 설명 |
|------|------|
| `PID_Init()` | Kp/Ki/Kd/MaxOutput 초기화 |
| `PID_Update()` | PID 연산 + Anti-windup |

## Anti-Windup 로직
```c
bool saturated_high = (output >= MaxOutput) && (error > 0);
bool saturated_low = (output <= 0) && (error < 0);
if (!saturated_high && !saturated_low) {
    IntegratedError += error * dt;
}
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **Anti-windup** | ⭐⭐⭐⭐⭐ | 조건부 적분 |
| **출력 클램핑** | ⭐⭐⭐⭐⭐ | 0~MaxOutput |
| **간결성** | ⭐⭐⭐⭐⭐ | 50줄 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**Anti-windup 및 출력 클램핑이 포함된 안정적인 PID 제어 알고리즘. 히터 온도 제어와 같은 열역학적 시스템 제어에 최적화된 간결하고 효율적인 구현임.**
