# FDIR Logic Unit Tests

이 폴더는 고장 진단 및 복구(`fdir.c`) 로직을 검증하기 위한 단위 테스트를 포함하고 있습니다.
외부 의존성(HAL, Sensors)을 Mocking하여 다양한 오류 상황을 시뮬레이션합니다.

## 1. 테스트 환경
- **컴파일러**: GCC
- **Mocking**: `mock_dependencies.c`, `main.h` (가짜 HAL)

## 2. 빌드 및 실행 방법

프로젝트 루트 디렉토리(`stm32_spaceballoon`)에서 아래 명령어를 실행하세요.
**주의**: `-DHOST_TEST_MODE` 플래그는 필수입니다 (실제 하드웨어 코드와 충돌 방지).

**Windows (PowerShell/CMD):**
```powershell
# 1. 컴파일
gcc -DHOST_TEST_MODE -I Core/Inc -I test/unity_minimal -I test/test_fdir -o test/test_fdir/test_fdir.exe test/test_fdir/test_main.c test/test_fdir/mock_dependencies.c Core/Src/fdir.c test/unity_minimal/unity.c

# 2. 실행
.\test\test_fdir\test_fdir.exe
```

## 3. 테스트 항목
| 테스트 함수명 | 설명 |
|---|---|
| `test_fdir_init_healthy` | 초기화 시 모든 센서가 Healthy 상태인지 확인 |
| `test_fdir_timeout_recovery` | 데이터 갱신 타임아웃 발생 시 RECOVERY 상태 진입 및 복구 시도 확인 |
| `test_fdir_permanent_failure` | 복구 시도 횟수 초과 시 FAILURE_PERMANENT 상태로 전환되는지 확인 |
| `test_fdir_cold_protection` | 저온(-20°C) 감지 시 센서 비활성화(Cold Disabled) 및 히스테리시스 복구 로직 확인 |

## 4. 실행 결과 예시
```text
Running test_fdir_init_healthy... PASS
Running test_fdir_timeout_recovery... PASS
Running test_fdir_permanent_failure... PASS
Running test_fdir_cold_protection... PASS
-------------------------
4 Tests Run, 0 Failed
```
