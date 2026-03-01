# Kalman Filter Unit Tests

이 폴더는 칼만 필터(`kalman.c`) 알고리즘의 동작을 검증하기 위한 단위 테스트를 포함하고 있습니다.
STM32 하드웨어 없이 PC(Windows/Linux/Mac)에서 GCC를 사용하여 논리적 정확성을 검증합니다.

## 1. 테스트 환경
- **컴파일러**: GCC (MinGW, Linux GCC 등)
- **프레임워크**: `../unity_minimal` (자체 제작한 최소 기능 Unity 호환 라이브러리 사용)

## 2. 빌드 및 실행 방법

프로젝트 루트 디렉토리(`stm32_spaceballoon`)에서 아래 명령어를 실행하세요.

**Windows (PowerShell/CMD):**
```powershell
# 1. 컴파일
gcc -I Core/Inc -I test/unity_minimal -o test/test_kalman/test_kalman.exe test/test_kalman/test_main.c Core/Src/kalman.c test/unity_minimal/unity.c

# 2. 실행
.\test\test_kalman\test_kalman.exe
```

## 3. 테스트 항목
| 테스트 함수명 | 설명 |
|---|---|
| `test_kf_init` | 초기 상태(고도 0, 속도 0) 및 공분산 행렬(P) 초기화 확인 |
| `test_kf_predict` | 예측 단계(Predict)에서 상태 변수(`x = Fx`)가 물리 법칙대로 갱신되는지 확인 |
| `test_kf_update_convergence` | 10m 고정 관측치를 지속적으로 입력했을 때, 필터 추정값이 10m로 수렴하는지 확인 |
| `test_kf_ascent_profile` | 가상의 5m/s 상승 데이터를 입력하여, 필터가 궤적을 잘 추적하고 속도를 추정하는지 확인 |

## 4. 실행 결과 예시
정상적으로 통과할 경우 아래와 같은 로그가 출력됩니다.

```text
Running test_kf_init... PASS
Running test_kf_predict... PASS
Running test_kf_update_convergence... PASS
Running test_kf_ascent_profile... PASS
-------------------------
4 Tests Run, 0 Failed
```
