# actuators.h 코드 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 23줄 |
| **역할** | 히터 액추에이터 인터페이스 |

## API

```c
void Actuators_Init(void);
void Actuators_SetHeater_Battery(float duty_percent); // 0~100%
void Actuators_SetHeater_Board(float duty_percent);
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **간결성** | ⭐⭐⭐⭐⭐ | 23줄 |
| **문서화** | ⭐⭐⭐⭐ | 핀 매핑 주석 |
| **C++ 호환** | ⭐⭐⭐⭐⭐ | extern "C" |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
