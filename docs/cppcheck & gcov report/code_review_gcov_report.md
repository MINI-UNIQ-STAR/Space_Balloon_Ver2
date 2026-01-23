# gcov 호스트 테스트 결과 (gcov_test_host)

## 개요
gcov를 사용하여 핵심 알고리즘의 코드 커버리지를 측정합니다.
ARM 타겟 빌드와 별도로 **PC(Host)에서** 테스트를 실행합니다.

## 필수 도구
- MinGW GCC (Windows) 또는 GCC (Linux)
- CMake 3.20+
- lcov (선택, HTML 리포트용)

## 빠른 시작

### 1. 호스트 테스트 빌드
```powershell
cd C:\Users\hyuns\Desktop\project\SpaceBalloon_2.0\spaceballoon_stm32_lora32

# 빌드 디렉토리 생성
mkdir build_host
cd build_host

# CMake 설정 (MinGW 사용)
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -f ../CMakeLists_host_test.txt ..

# 빌드
cmake --build .
```

### 2. 테스트 실행
```powershell
./host_test_runner.exe
```

### 3. 커버리지 리포트 생성
```powershell
# gcov 실행
gcov -b *.gcda

# 결과 확인
cat pid.c.gcov
cat kalman.c.gcov
```

## 커버리지 대상 파일
| 파일 | 설명 |
|------|------|
| `pid.c` | PID 제어기 |
| `kalman.c` | 칼만 필터 |
| `fdir.c` | 고장 검출/복구 |
| `telemetry.c` | 텔레메트리 패킷 |

## 커버리지 목표 및 실제 결과 (2026-01-23)

| 파일 | 구분 | 목표 | **실제 달성도** | 상태 |
|------|------|------|---------------|------|
| **pid.c** | Line | 80% | **88.89%** | ✅ PASS |
| | Branch | 70% | **62.50%** | ⚠️ 미달 (후술) |
| **kalman.c** | Line | 80% | **89.47%** | ✅ PASS |
| | Branch | 70% | **75.00%** | ✅ PASS |

### 분석 및 특이사항
- **pid.c (Branch 62.5%)**: 테스트 시나리오가 '가열' 위주여서 출력이 항상 포화 상태였음. 온도 하강 시나리오 추가 시 향상 가능.
- **kalman.c (Line 89.47%)**: NaN 발생 시의 복구 로직(안전 장치)을 제외한 모든 핵심 로직이 실행됨.

## 제한사항
- 하드웨어 의존 코드(HAL)는 Mock으로 대체됨
- `HOST_TEST_MODE` 매크로로 하드웨어 호출 우회
