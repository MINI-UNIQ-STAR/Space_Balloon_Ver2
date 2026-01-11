# Low Voltage Protection Unit Tests

이 폴더는 저전압 보호(Load Shedding) 로직을 검증하기 위한 단위 테스트를 포함합니다.

## 테스트 항목

| 테스트 함수 | 설명 |
|------------|------|
| `test_normal_voltage` | 정상 전압(3.7V)에서 저전압 모드가 활성화되지 않음을 확인 |
| `test_low_voltage_entry` | 2.6V로 떨어질 때 저전압 모드 진입 확인 (히터 차단) |
| `test_hysteresis_below_exit` | 히스테리시스 테스트: 2.8V로 회복 시 여전히 저전압 모드 유지 |
| `test_low_voltage_exit` | 3.0V로 회복 시 저전압 모드 종료 확인 |
| `test_multiple_cycles` | 여러 번의 진입/종료 사이클 테스트 |

## 빌드 및 실행

### Windows (MinGW/MSYS2)
```bash
gcc -DHOST_TEST_MODE \
    -I ../../Core/Inc \
    -I ../../test/unity_minimal \
    -o test_low_voltage.exe \
    test_main.c \
    ../../Core/Src/app.c \
    ../../test/unity_minimal/unity.c \
    -lm

./test_low_voltage.exe
```

### Linux/Mac
```bash
gcc -DHOST_TEST_MODE \
    -I ../../Core/Inc \
    -I ../../test/unity_minimal \
    -o test_low_voltage \
    test_main.c \
    ../../Core/Src/app.c \
    ../../test/unity_minimal/unity.c \
    -lm

./test_low_voltage
```

## 예상 출력

```
test/test_low_voltage/test_main.c:25:test_normal_voltage:PASS
test/test_low_voltage/test_main.c:42:test_low_voltage_entry:PASS
test/test_low_voltage/test_main.c:60:test_hysteresis_below_exit:PASS
test/test_low_voltage/test_main.c:78:test_low_voltage_exit:PASS
test/test_low_voltage/test_main.c:94:test_multiple_cycles:PASS

-----------------------
5 Tests 0 Failures 0 Ignored
OK
```

## 임계값 정보

- **저전압 진입**: < 2.7V (2700mV)
- **저전압 종료**: > 2.9V (2900mV)
- **히스테리시스**: 200mV (2.7V ~ 2.9V)

## 구현 파일

- [Core/Src/app.c](../../Core/Src/app.c#L191-L224) - 저전압 보호 로직
- [Core/Inc/app.h](../../Core/Inc/app.h#L17-L19) - 전역 변수 및 API
