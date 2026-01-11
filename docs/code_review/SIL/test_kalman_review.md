# test_kalman 단위 테스트 코드 리뷰

| 항목 | 내용 |
|------|------|
| **프레임워크** | Unity |
| **테스트 수** | 4개 |
| **대상** | `kalman.c` |

---

## 테스트 케이스

| 테스트 | 검증 항목 |
|--------|----------|
| `test_kf_init` | 상태/공분산 초기화 |
| `test_kf_predict` | 상태 예측 (x + v*dt) |
| `test_kf_update_convergence` | 측정값 수렴 |
| `test_kf_ascent_profile` | 상승 프로파일 추적 |

---

## 상승 프로파일 테스트

```c
void test_kf_ascent_profile(void) {
    // 5m/s 상승 시뮬레이션
    for(int i=0; i<100; i++) {
        true_alt += vel * dt;
        KF_Predict(&hkf);
        KF_Update_Altitude(&hkf, true_alt);
    }
    // 고도 추적: ±2m 허용
    TEST_ASSERT_FLOAT_WITHIN(2.0f, true_alt, hkf.x[0]);
    // 속도 추정: ±1m/s 허용
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 5.0f, hkf.x[1]);
}
```

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**KF 수렴 및 상승 프로파일 추적 검증.**
