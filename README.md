# stm32_spaceballoon (STM32G431 + STM32Cube HAL + PlatformIO)

이 저장소는 STM32G431(PlatformIO + STM32Cube HAL)을 기반으로, 여러 센서 데이터를 수집해 **USART3**로 ESP32에 **고정 바이너리 프레임(50Hz)** 로 전송하는 프로젝트입니다.

## 빠른 시작

### 빌드

```bash
pio run
```

### 업로드

```bash
pio run -t upload
```

### 시리얼 모니터

```bash
pio device monitor
```

### 유닛 테스트(호스트 네이티브)

HAL 의존 없는 순수 C 로직(프레이밍/파서/보정수식 등)은 PC에서 Unity로 테스트합니다.

```bash
pio test -e native_test
```

## 프로젝트 구조 (중요)

- CubeMX가 **source of truth** 입니다: `stm32_spaceballoon.ioc` → `Core/` 생성
- PlatformIO는 CubeMX 출력으로 직접 빌드합니다: `platformio.ini`의 `src_dir = Core/Src`, `include_dir = Core/Inc`

추가로, `src/` 폴더는 문서용입니다. 예전에 `src/` 아래에 있던 중복 소스들은 혼동 방지를 위해 `archive/duplicate_src_YYYY-MM-DD/`로 이동했습니다.

권장 계층(의존성 단방향): **app → services → drivers → HAL**

- 애플리케이션: `Core/Src/app/`, `Core/Inc/app/`
- 서비스 레이어: `Core/Src/services/`, `Core/Inc/services/`
- 드라이버 레이어: `Core/Src/drivers/`, `Core/Inc/drivers/`

CubeMX가 생성하는 파일을 수정할 때는 반드시 `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` 블록 안에서만 변경하세요.

## 버스/포트 매핑(프로젝트 규칙)

PDF/레퍼런스 문서의 핀(PAxx/PCxx 등) 정보가 실제와 다를 수 있습니다. **핀/AF/NVIC는 `stm32_spaceballoon.ioc`와 생성된 `Core/Src/stm32g4xx_hal_msp.c`를 기준**으로 합니다.

### I2C

- I2C1: GDK101, LSM6DSV16x, MLX90393
- I2C3: CM1107N, MCP9600, SHT31-D, MS5611
  - 참고: 일부 문서에 I2C2라고 적혀 있어도, 이 코드베이스에서는 **I2C3** 를 사용합니다.

### UART

- USART1: XA1110 (GPS)
- USART2: PMS3003
- USART3: 텔레메트리 출력(ESP32)

`printf`를 UART에 무심코 붙이지 마세요(역할 충돌 가능).

## 텔레메트리(USART3 → ESP32)

- 50Hz 고정 스냅샷 프레임 송신
- 프레이밍/CRC는 `Core/Inc/services/telemetry_frame.h` 참고
- CRC: **CRC-16/CCITT-FALSE**
- 멀티바이트는 little-endian(구조체 memcpy 기반)
- float 대신 정수 고정소수점(SI 기반 스케일) 사용

스냅샷 payload에는 현재 다음 값들이 포함됩니다(확장됨):
- GPS: 위도/경도/고도, fix, 위성 카운트
- 배터리: `bat_mv`
- 내부 온도: DS18B20 (`temp_c_x100`)
- 공기질: PMS3003 (`pm1/pm2.5/pm10`)
- 외기 온습도: SHT31-D (`sht31_temp_c_x100`, `sht31_rh_x100`)
- 기압/온도/고도: MS5611 (`ms5611_press_pa`, `ms5611_temp_c_x100`, `ms5611_alt_m`)

## 현재 포함된 센서/서비스

- GPS (XA1110 / USART1): NMEA 파서 + 서비스
- PMS3003 (USART2): 스트리밍 프레임 파서 + 서비스
- DS18B20 (1-Wire GPIO): 드라이버 + 1Hz 서비스
- Battery ADC (ADC1_IN2): 드라이버 + 1Hz 서비스
- SHT31-D (I2C3): 드라이버 + 1Hz 서비스
- MS5611 (I2C3): 드라이버 + 약 10Hz 서비스

### LSM6DSV16x (IMU)

- 텔레메트리의 `accel_mps2_x1000[3]`, `gyro_rads_x1000[3]`는 IMU에서 읽은 XYZ를 그대로 채웁니다.
- **주의:** IMU의 XYZ 축 방향은 “센서 칩 기준”이며, 보드 장착 방향/좌표계 정의에 맞춰 **XYZ 축 매핑(및 부호)** 를 후속으로 조정해야 합니다.

## 센서 주소/설정 팁

### SHT31-D

- 기본 7-bit 주소는 보통 `0x44` (ADDR low) 입니다.
- 변환/CRC 로직은 `drivers/sht31_codec.*` 에 분리되어 있어 유닛 테스트가 가능합니다.

### MS5611

- I2C 7-bit 주소는 `111011Cx` 형태이며, `C = ~CSB` 입니다.
  - CSB=VDD → 주소 `0x76`
  - CSB=GND → 주소 `0x77`
- 현재 기본값은 `0x76`이며, 필요 시 `MS5611_I2C_ADDR_7BIT` 매크로로 변경 가능합니다.
- 보정 수식(1차+2차) 및 PROM CRC4는 `drivers/ms5611_codec.*` 에 있습니다(데이터시트 예제로 유닛 테스트 포함).

고도(altitude)는 표준대기 근사식으로 계산해 `ms5611_alt_m`(m)로 전송합니다. 해수면 기준압은 기본 `101325Pa`이며, 필요 시 `MS5611_P0_PA` 매크로로 변경 가능합니다.

## 개발 가이드

- 새로운 센서 추가는 “드라이버 + 서비스 + 텔레메트리 필드 + (가능하면) native 유닛 테스트”를 한 세트로 추가하는 방식이 가장 안전합니다.
- HAL 의존 부분은 `native_test`에 넣지 말고, 순수 로직(프레이밍/파서/보정/CRC)은 codec 모듈로 분리해 테스트하세요.
- 메모리 제약: Flash/RAM이 작기 때문에 큰 버퍼/정적 데이터 추가에 주의하세요.

---

### 참고

- 자세한 작업 규칙/컨벤션은 `.github/copilot-instructions.md`에 정리되어 있습니다.
