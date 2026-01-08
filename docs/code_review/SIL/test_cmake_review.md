# test 빌드 시스템 코드 리뷰

| 항목 | 내용 |
|------|------|
| **빌드 도구** | CMake 3.10+ |
| **테스트 프레임워크** | Unity (Minimal) |
| **코드 라인** | 142줄 |

---

## 테스트 러너 목록

| 타겟 | 소스 | 대상 |
|------|------|------|
| `test_pid_runner` | pid.c | PID 제어기 |
| `test_kalman_runner` | kalman.c | 칼만 필터 |
| `test_fdir_runner` | fdir.c | 결함 감지 |
| `test_drivers_runner` | 12개 드라이버 | 센서 드라이버 |
| `test_integration_runner` | app.c + 전체 | 미션 시뮬레이션 |

---

## 컴파일 정의
```cmake
add_definitions(-DUNIT_TEST)
target_compile_definitions(test_*_runner PRIVATE HOST_TEST_MODE)
```

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**완전한 CMake 기반 테스트 인프라.**
