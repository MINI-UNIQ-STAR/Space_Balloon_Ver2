# Renode 시뮬레이션 빠른 시작 가이드

## 1. 준비사항 확인

### 필수 소프트웨어

- [x] **Renode**: https://renode.io/ 에서 다운로드
- [x] **ARM GCC Toolchain**: arm-none-eabi-gcc
- [x] **cube-cmake**: STM32CubeMX CMake 지원

### 설치 확인

```powershell
# Renode
renode --version

# ARM GCC
arm-none-eabi-gcc --version

# cube-cmake
cube-cmake --version
```

## 2. 빌드 및 실행 (자동)

가장 쉬운 방법:

```powershell
# 프로젝트 루트에서
.\renode\run_simulation.ps1
```

이 스크립트는:
1. Renode 설치 확인
2. 펌웨어 빌드 (Debug 모드)
3. Renode 시뮬레이션 시작

### 옵션

```powershell
# 빌드만 하기
.\renode\run_simulation.ps1 -BuildOnly

# Release 빌드로 시뮬레이션
.\renode\run_simulation.ps1 -BuildType Release

# 빌드 건너뛰고 시뮬레이션만
.\renode\run_simulation.ps1 -SimOnly
```

## 3. 수동 실행

### 3.1 펌웨어 빌드

```powershell
# CMake 설정
cube-cmake -B build/Debug -G "MinGW Makefiles"

# 빌드
cube-cmake --build build/Debug
```

### 3.2 Renode 실행

```powershell
# Renode 시작
renode

# Renode 콘솔에서 스크립트 로드
(monitor) i @renode/simulation.resc

# 시뮬레이션 시작
(monitor) start
```

## 4. 시뮬레이션 사용법

### 기본 명령어

```
# 시뮬레이션 시작/일시정지
(monitor) start
(monitor) pause

# 상태 확인
(monitor) machine

# 페리페럴 목록
(monitor) peripherals

# UART 출력 보기
(monitor) showAnalyzer sysbus.usart1
```

### 센서 데이터 확인

I2C 센서 통신 로그 활성화:

```
(monitor) logLevel 3 sysbus.i2c1
(monitor) logLevel 3 sysbus.i2c3
```

### GDB 디버거 연결

Renode에서:

```
(monitor) machine StartGdbServer 3333
```

별도 터미널에서:

```powershell
arm-none-eabi-gdb build/Debug/stm32_spaceballoon.elf
(gdb) target remote localhost:3333
(gdb) b main
(gdb) c
```

## 5. 시뮬레이션 확인 포인트

시뮬레이션이 제대로 실행되는지 확인:

### ✓ 시스템 초기화

UART 출력에서 다음을 확인:

```
[App] System Init
[Sensors] Initializing...
```

### ✓ 센서 통신

I2C 로그에서 센서 읽기/쓰기 확인:

```
[I2C1] Write to 0x6B: 0x0F (LSM6DSV16X WHO_AM_I)
[I2C1] Read from 0x6B: 0x70
```

### ✓ GPS NMEA 데이터

UART1 분석기에서 NMEA 문장 확인:

```
$GPGGA,123456.00,3505.5788,N,12659.9322,E,1,09,1.0,100.0,M,0.0,M,,*XX
```

## 6. 문제 해결

### 문제: "Platform not found"

**원인**: 플랫폼 파일 경로가 잘못됨

**해결**:
```powershell
# 절대 경로 사용
(monitor) machine LoadPlatformDescription @C:/full/path/to/renode/stm32g431.repl
```

### 문제: "ELF not found"

**원인**: 펌웨어가 빌드되지 않음

**해결**:
```powershell
cube-cmake --build build/Debug
```

### 문제: Python 센서 로드 실패

**원인**: Renode는 IronPython을 사용하므로 일부 Python 기능 제한

**해결**:
- `import` 문에서 표준 라이브러리만 사용
- `numpy`, `scipy` 같은 외부 라이브러리는 사용 불가
- 대신 `math` 모듈 사용

### 문제: 시뮬레이션이 느림

**해결**:
```
# 터보 모드 활성화
(monitor) emulation SetGlobalQuantum "0.0001"
```

## 7. 다음 단계

시뮬레이션이 정상 작동하면:

1. **센서 데이터 분석**: 텔레메트리 패킷 확인
2. **알고리즘 검증**: Kalman 필터, PID 제어 동작 확인
3. **FDIR 테스트**: 센서 오류 주입 및 복구 확인
4. **전력 관리**: 저전압 시나리오 시뮬레이션

## 8. 참고 자료

- [Renode 문서](https://renode.readthedocs.io/)
- [STM32G4 데이터시트](https://www.st.com/resource/en/datasheet/stm32g431cb.pdf)
- [프로젝트 README](../README.md)
