# PID Controller Unit Tests

이 폴더는 PID 제어기(`pid.c`) 알고리즘의 동작을 검증하기 위한 단위 테스트를 포함하고 있습니다.
각 제어 항(P, I, D)의 계산 정확성과 출력 제한(Clamping) 로직을 검증합니다.

## 1. 테스트 환경
- **컴파일러**: GCC
- **프레임워크**: `../unity_minimal`

## 2. 빌드 및 실행 방법

프로젝트 루트 디렉토리(`stm32_spaceballoon`)에서 아래 명령어를 실행하세요.

**Windows (PowerShell/CMD):**
```powershell
# 1. 컴파일
gcc -I Core/Inc -I test/unity_minimal -o test/test_pid/test_pid.exe test/test_pid/test_main.c Core/Src/pid.c test/unity_minimal/unity.c

# 2. 실행
.\test\test_pid\test_pid.exe
```

## 3. 테스트 항목
| 테스트 함수명 | 설명 |
|---|---|
| `test_pid_init` | PID 구조체 초기화 및 게인값 설정 확인 |
| `test_pid_p_term` | 비례항(Proportional) 계산 검증 (Output = Kp * Error) |
| `test_pid_i_term` | 적분항(Integral) 누적 및 계산 검증 (IntError += Error * dt) |
| `test_pid_d_term` | 미분항(Derivative) 변화율 계산 검증 (Derivative = (Error - LastError) / dt) |
| `test_pid_clamping` | 출력값 제한(MaxOutput 및 0.0f 하한) 로직 검증 (히터 제어용) |

## 4. 실행 결과 예시
```text
Running test_pid_init... PASS
Running test_pid_p_term... PASS
Running test_pid_i_term... PASS
Running test_pid_d_term... PASS
Running test_pid_clamping... PASS
-------------------------
5 Tests Run, 0 Failed
```
