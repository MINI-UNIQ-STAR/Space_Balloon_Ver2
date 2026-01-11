# fdir.h FDIR 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 79줄 |
| **역할** | 결함 감지/복구 인터페이스 |

## 상태 타입
```c
typedef enum {
    FDIR_STATE_HEALTHY,
    FDIR_STATE_WARNING,
    FDIR_STATE_RECOVERY,
    FDIR_STATE_FAILURE_PERMANENT
} FdirState_t;
```

## 센서 건강 구조체
```c
typedef struct {
    uint32_t last_valid_update_ms;
    uint32_t error_count, recovery_count;
    FdirState_t state;
    bool enabled;
} SensorHealth_t;
```

## 상태 플래그

| 비트 | 플래그 |
|------|------|
| 0 | SYS_OK |
| 1 | GPS_WARN |
| 2 | BARO_WARN |
| 5 | HEATER_ACTIVE |
| 8 | ALT_JUMP |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **타입 정의** | ⭐⭐⭐⭐⭐ | enum/struct |
| **비트 플래그** | ⭐⭐⭐⭐⭐ | 명확한 정의 |
| **검증 API** | ⭐⭐⭐⭐⭐ | ValidateRange_* |

## 최근 개선사항
- ✅ **C4819 경고 해결**: UTF-8 BOM 제거, ASCII 주석으로 변경
- ✅ **MSVC 빌드 호환성**: Windows 환경에서 경고 없이 컴파일

## 종합: ⭐⭐⭐⭐⭐ (5/5)
