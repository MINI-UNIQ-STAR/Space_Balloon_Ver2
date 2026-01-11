# app.h 코드 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 21줄 |
| **역할** | 애플리케이션 인터페이스 |

## 공개 핸들
```c
extern PID_HandleTypeDef hpid_bat, hpid_brd;
extern KF_Handle_t hkf;
extern telemetry_frame_t telem_frame;
extern float heater_battery_cmd, heater_board_cmd;
```

## API
```c
void App_Init(void);
void App_Loop(void);
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **테스트 접근성** | ⭐⭐⭐⭐⭐ | extern 핸들 |
| **의존성** | ⭐⭐⭐⭐⭐ | pid/kalman/telemetry 포함 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
