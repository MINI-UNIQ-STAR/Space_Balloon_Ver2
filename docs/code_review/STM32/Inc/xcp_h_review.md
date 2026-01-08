# xcp.h XCP 프로토콜 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 29줄 |
| **역할** | XCP 캘리브레이션 인터페이스 |

## ODT 엔트리
```c
typedef struct {
    float *ptr;
    uint8_t size, type;
} XCP_ODT_Entry_t;
```

## API
```c
void XCP_Init(void);
void XCP_ProcessCommand(data, len);
void XCP_UpdateMeasurements(void);
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **준비 상태** | ⭐⭐⭐⭐ | PID/KF extern |
| **완성도** | ⭐⭐⭐ | 스텁 |

## 종합: ⭐⭐⭐⭐ (4/5)
