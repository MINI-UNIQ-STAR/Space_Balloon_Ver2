# test_integration 통합 테스트 리뷰

| 항목 | 내용 |
|------|------|
| **프레임워크** | Unity |
| **대상** | 전체 미션 흐름 |
| **파일** | test_mission.c |

---

## 구성

```cmake
# app.c + 전체 서비스 레이어 + HostSim Mock
add_executable(test_integration_runner
    test_integration/test_mission.c
    ../Core/Src/app.c
    ../Core/Src/actuators.c
    ../Core/Src/fdir.c
    ../Core/Src/kalman.c
    ../Core/Src/pid.c
    ../Core/Src/telemetry.c
    ../Core/Src/telemetry.c
    ../Core/Src/xcp.c
    ../HostSim/mock_sensors.c
    ../HostSim/mock_hal.c
    ../test_integration/bsp.c # (Stubbed inside test_mission.c currently)
)
```

---

## 테스트 시나리오

- RS41 비행 데이터 재생
- 전체 센서 → 필터 → 제어 → 텔레메트리 흐름
- SITL (Software-in-the-Loop) 완전 검증

---

## 출력 로그
`sitl_debug_log.txt` (1.4MB): 상세 시뮬레이션 로그

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**End-to-End SITL 검증.**
