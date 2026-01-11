# SIL/MSVC 및 Mock 개선 보고서

**날짜**: 2026-01-08
**작성자**: Antigravity

---

## 1. 개요
본 문서는 2026-01-08에 수행된 SIL(Testing Software-in-the-Loop) 환경의 호환성 개선, 드라이버 최적화, 그리고 HIL Mock의 기능 확장에 대한 상세 내용을 기술합니다.

## 2. 주요 개선 사항

### 2.1 SIL/MSVC 빌드 호환성 (IMP-06)
Visual Studio (MSVC) 컴파일러 환경에서 `test_drivers` 및 `test_integration` 타겟 빌드 시 발생하던 경고와 에러를 모두 해결했습니다.

| 구분 | 이슈 | 해결 방안 |
|------|------|-----------|
| **경고** | C4305 (double to float) | 리터럴에 `f` 접미사 추가 (`0.1` -> `0.1f`) |
| **경고** | C4101 (Unused variable) | 미사용 로컬 변수 제거 또는 주석 처리 |
| **경고** | C4267 (Data loss) | `size_t` -> `uint16_t` 명시적 캐스팅 추가 |
| **경고** | C4819 (Encoding) | 비표준 문자 제거 및 `#pragma warning(disable:4819)` 적용 |
| **에러** | LNK2019 (Linker) | `test_mission.c`에 BSP 스텁 함수 (`BSP_Init` 등) 구현 |

### 2.2 성능 최적화 (IMP-10)
`XA1110` GPS 드라이버의 파싱 효율을 높이기 위해 불필요한 NMEA 문장 처리를 제외했습니다.
- **변경**: `XA1110_ParseSentence`에서 `GSA` (DOP), `GSV` (위성 정보) 파싱 비활성화.
- **효과**: 인터럽트 처리에 소요되는 CPU 사이클 절감.

### 2.3 오류 처리 강화 (IMP-08)
`GDK101` 감마선 센서 드라이버의 초기화 로직을 개선했습니다.
- **변경**: 초기화 시 소프트 리셋 수행 후 상태 레지스터 확인.
- **결과**: 초기화 실패 원인을 '리셋 실패'(`GDK101_RESET_ERR`)와 '통신 실패'(`GDK101_I2C_ERR`)로 구분 가능.

### 2.4 HIL Mock 고도화 (IMP-03, IMP-04)
Arduino 기반의 HIL 시뮬레이터가 실제 센서와 더 유사하게 동작하도록 개선했습니다.
- **LSM6DSV16X (IMU)**: 256바이트 가상 레지스터 맵 구현. I2C 쓰기/읽기를 통해 설정 값 유지 및 센서 데이터(가속도/자이로) 반환.
- **MS5611 (Baro)**: 역산 로직(Inverse Calculation) 추가. 입력된 온도/기압(`mock_temp`, `mock_press`)을 ADC 원시 값(`D1`, `D2`)으로 변환하여 제공.

### 2.5 XCP 프로토콜 구현 (IMP-05)
UART를 통한 기본적인 캘리브레이션 및 측정 프로토콜(XCP)을 구현했습니다.
- **구현 명령어**:
    - `CONNECT (0xFF)`: 연결 수립
    - `SHORT_UPLOAD (0xF4)`: 메모리 읽기 (변수 모니터링)
    - `SHORT_DOWNLOAD (0xF0)`: 메모리 쓰기 (파라미터 튜닝)

---

## 3. 검증 결과

### 3.1 자동화 테스트
- **test_drivers**: 빌드 성공 (Warnings: 0, Errors: 0)
- **test_integration**: 빌드 성공 (Warnings: 0, Errors: 0), 링킹 정상 확인.

### 3.2 수동 리뷰
- **Mock 동작**: Arduino 코드는 ESP-NOW 수신 데이터와 I2C 요청 간의 동기화 로직이 정상적으로 구현됨을 확인.
