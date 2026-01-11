# GPS NMEA Checksum Validation Tests

이 폴더는 GPS NMEA 문장 체크섬 검증 로직을 테스트합니다.

## 테스트 항목

| 테스트 함수 | 설명 |
|------------|------|
| `test_valid_gga_sentence` | 유효한 GGA 문장 (올바른 체크섬) 파싱 성공 확인 |
| `test_invalid_checksum` | 잘못된 체크섬을 가진 문장 거부 확인 |
| `test_valid_rmc_sentence` | 유효한 RMC 문장 파싱 및 데이터 추출 확인 |
| `test_malformed_no_dollar` | $ 기호 없는 malformed 문장 거부 |
| `test_valid_gsv_gps` | GPS 위성 정보 (GSV) 파싱 확인 |
| `test_valid_gsv_glonass` | GLONASS 위성 정보 파싱 확인 |
| `test_corrupted_data` | 손상된 데이터 (non-printable) 거부 |
| `test_empty_sentence` | 빈 문장 거부 |
| `test_no_checksum_lenient` | 체크섬 없는 문장 처리 (lenient mode) |

## 빌드 및 실행

### Windows (MinGW/MSYS2)
```bash
gcc -DHOST_TEST_MODE \
    -I ../../Core/Inc \
    -I ../../Core/Drivers/xa1110 \
    -I ../../Core/Drivers/minmea \
    -I ../../test/unity_minimal \
    -o test_gps_checksum.exe \
    test_main.c \
    ../../Core/Drivers/xa1110/xa1110_driver.c \
    ../../Core/Drivers/minmea/minmea.c \
    ../../test/unity_minimal/unity.c \
    -lm

./test_gps_checksum.exe
```

### Linux/Mac
```bash
gcc -DHOST_TEST_MODE \
    -I ../../Core/Inc \
    -I ../../Core/Drivers/xa1110 \
    -I ../../Core/Drivers/minmea \
    -I ../../test/unity_minimal \
    -o test_gps_checksum \
    test_main.c \
    ../../Core/Drivers/xa1110/xa1110_driver.c \
    ../../Core/Drivers/minmea/minmea.c \
    ../../test/unity_minimal/unity.c \
    -lm

./test_gps_checksum
```

## 예상 출력

```
test/test_gps_checksum/test_main.c:21:test_valid_gga_sentence:PASS
test/test_gps_checksum/test_main.c:34:test_invalid_checksum:PASS
test/test_gps_checksum/test_main.c:45:test_valid_rmc_sentence:PASS
test/test_gps_checksum/test_main.c:58:test_malformed_no_dollar:PASS
test/test_gps_checksum/test_main.c:69:test_valid_gsv_gps:PASS
test/test_gps_checksum/test_main.c:81:test_valid_gsv_glonass:PASS
test/test_gps_checksum/test_main.c:93:test_corrupted_data:PASS
test/test_gps_checksum/test_main.c:103:test_empty_sentence:PASS
test/test_gps_checksum/test_main.c:113:test_no_checksum_lenient:PASS

-----------------------
9 Tests 0 Failures 0 Ignored
OK
```

## 체크섬 계산 방법

NMEA 체크섬은 `$`와 `*` 사이의 모든 바이트를 XOR한 값입니다.

예: `$GPGGA,...*47`
- `47`은 16진수 체크섬
- `G^P^G^G^A^,...` = 0x47

## 테스트 문장 예시

### 유효한 GGA 문장
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
```

### 잘못된 체크섬
```
$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48
```
(마지막 숫자가 7→8로 변경됨)

## 구현 파일

- [Core/Drivers/xa1110/xa1110_driver.c](../../Core/Drivers/xa1110/xa1110_driver.c#L48-L52) - 체크섬 검증 추가
- [Core/Drivers/minmea/minmea.c](../../Core/Drivers/minmea/minmea.c) - minmea 라이브러리 (체크섬 계산)

## Multi-GNSS 지원

- **GPS** (GP): Talker ID `GP`
- **GLONASS** (GL): Talker ID `GL`
- **Galileo** (GA): Talker ID `GA`
- **BeiDou** (GB): Talker ID `GB`
