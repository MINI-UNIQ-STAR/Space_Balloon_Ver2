# gcov 호스트 테스트 (gcov_test_host)

## 개요
PC에서 핵심 알고리즘(PID, Kalman)을 테스트하고 **gcov 코드 커버리지**를 측정하기 위한 호스트 빌드 환경입니다.

## 필요 환경
- **MSYS2 MinGW 64-bit** (Windows)
- CMake 3.20+
- GCC (mingw-w64-x86_64-gcc)

## 빠른 시작

### 1. MSYS2 MinGW 64-bit 터미널 열기
Windows 시작 메뉴 → "MSYS2 MinGW 64-bit" 실행

### 2. 빌드
```bash
cd /c/Users/hyuns/Desktop/project/SpaceBalloon_2.0/spaceballoon_stm32_lora32
mkdir -p build_host && cd build_host
rm -rf *
cmake -G "MinGW Makefiles" ../gcov_test_host
mingw32-make
```

### 3. 테스트 실행
```bash
./host_test_runner.exe
```

### 4. 커버리지 측정
```bash
# 커버리지 파일 찾기
find . -name "*.gcda"

# PID 커버리지
gcov -b CMakeFiles/host_test_runner.dir/C_/Users/hyuns/Desktop/project/SpaceBalloon_2.0/spaceballoon_stm32_lora32/Core/Src/pid.c.gcda

# Kalman 커버리지
gcov -b CMakeFiles/host_test_runner.dir/C_/Users/hyuns/Desktop/project/SpaceBalloon_2.0/spaceballoon_stm32_lora32/Core/Src/kalman.c.gcda

# 결과 확인
cat pid.c.gcov
cat kalman.c.gcov
```

## 테스트 대상

| 파일 | 설명 | 예상 커버리지 |
|------|------|--------------|
| `pid.c` | PID 제어기 (Anti-windup) | 85%+ |
| `kalman.c` | 칼만 필터 (발산 방지) | 85%+ |

## 파일 구조
```
gcov_test_host/
├── CMakeLists.txt      # 빌드 설정
├── test_algorithms.c   # 테스트 메인
└── README.md           # 이 파일
```

## 최근 테스트 결과 (2026-01-23)
- **PID Controller**: ✅ PASS (Line 88.89%, Branch 62.5%)
- **Kalman Filter**: ✅ PASS (Line 89.47%, Branch 75.0%)
- **Divergence Reset**: ✅ PASS
