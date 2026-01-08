# pid.h PID 제어기 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 29줄 |
| **역할** | PID 제어기 인터페이스 |

## 핸들 구조체
```c
typedef struct {
    float Kp, Ki, Kd, MaxOutput;
    float Target, IntegratedError, LastError;
} PID_HandleTypeDef;
```

## API
```c
void PID_Init(hpid, Kp, Ki, Kd, MaxOutput);
float PID_Update(hpid, measurement, dt);
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **HAL 스타일** | ⭐⭐⭐⭐⭐ | HandleTypeDef |
| **간결성** | ⭐⭐⭐⭐⭐ | 29줄 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
