# kalman.h 칼만 필터 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 40줄 |
| **역할** | 칼만 필터 인터페이스 |

## 상태 벡터 구조체
```c
typedef struct {
    float x[2];      // [고도, 속도]
    float P[2][2];   // 공분산
    float Q[2][2];   // 프로세스 노이즈
    float R;         // 측정 노이즈
    float dt;        // 시간 간격
} KF_Handle_t;
```

## API
```c
void KF_Init(hkf, dt, process_noise, meas_noise);
void KF_Predict(hkf);
void KF_Update_Altitude(hkf, measurement);
void KF_CheckDivergence(hkf);  // FMEA 대응
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **구조체 설계** | ⭐⭐⭐⭐⭐ | 완전한 KF 상태 |
| **안전 API** | ⭐⭐⭐⭐⭐ | CheckDivergence |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
