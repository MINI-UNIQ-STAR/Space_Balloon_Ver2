# test_fdir 단위 테스트 코드 리뷰

| 항목 | 내용 |
|------|------|
| **프레임워크** | Unity |
| **테스트 수** | 4개 |
| **대상** | `fdir.c` |

---

## 테스트 케이스

| 테스트 | 검증 항목 |
|--------|----------|
| `test_fdir_init_healthy` | 모든 센서 HEALTHY 초기화 |
| `test_fdir_timeout_recovery` | 타임아웃 → RECOVERY 전이 |
| `test_fdir_permanent_failure` | 최대 복구 시도 후 PERMANENT |
| `test_fdir_cold_protection` | 저온 센서 비활성화/히스테리시스 |

---

## 핵심 테스트

### 타임아웃 복구
```c
MockHAL_AdvanceTick(1100);  // Baro timeout=1000ms
FDIR_Update();
TEST_ASSERT_EQUAL_INT(FDIR_STATE_RECOVERY, state);
TEST_ASSERT_EQUAL_INT(target, MockSensors_GetLastResetSensor());
```

### 저온 보호
```c
FDIR_UpdateTemperature(-2000);  // -20°C
FDIR_Update();
TEST_ASSERT_TRUE(FDIR_IsSensorColdDisabled(SENSOR_ID_PMS));
```

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**FDIR 상태머신 + 저온 보호 검증.**
