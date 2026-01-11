# test_pid 단위 테스트 코드 리뷰

| 항목 | 내용 |
|------|------|
| **프레임워크** | Unity |
| **테스트 수** | 6개 |
| **대상** | `pid.c` |

---

## 테스트 케이스

| 테스트 | 검증 항목 |
|--------|----------|
| `test_pid_init` | Kp/Ki/Kd/MaxOutput 초기화 |
| `test_pid_p_term` | 비례항 출력 검증 |
| `test_pid_i_term` | 적분항 누적 |
| `test_pid_d_term` | 미분항 계산 |
| `test_pid_clamping` | 0~MaxOutput 클램핑 |
| `test_pid_anti_windup` | 포화 시 적분 억제 |

---

## Anti-Windup 테스트

```c
void test_pid_anti_windup(void) {
    // Kp=100, Max=50 → 출력 포화
    PID_Update(&hpid, 0.0f, 1.0f);  // P = 10000 → 포화!
    // I-term이 누적되지 않아야 함
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, ie1);
}
```

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**완전한 PID 검증. Anti-windup 핵심 테스트 포함.**
