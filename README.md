# stm32_spaceballoon (STM32G431 + STM32Cube HAL + PlatformIO)

이 저장소는 STM32G431(PlatformIO + STM32Cube HAL + FreeRTOS)을 기반으로, 여러 센서 데이터를 수집해 **USART3**로 ESP32에 **고정 바이너리 프레임(50Hz)** 로 전송하는 프로젝트입니다.

핵심 목표는 “센서 I2C 지연/리커버리 등으로 시스템이 흔들려도” **50Hz 텔레메트리 주기를 안정적으로 유지**하는 것입니다.

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

### 전송 경로(중요)

- USART3 텔레메트리 TX는 **비차단(IRQ 기반 링버퍼)** 방식으로 동작합니다.
  - 115200bps에서 프레임 크기가 커질 경우, 블로킹 전송은 20ms 주기(50Hz)에 치명적일 수 있어 비차단으로 설계되어 있습니다.
- “송신(50Hz)”과 “센서 읽기(느릴 수 있음)”를 분리하기 위해, 텔레메트리는 **스냅샷 payload**를 읽어 전송합니다.

스냅샷 payload에는 현재 다음 값들이 포함됩니다(확장됨):
- GPS: 위도/경도/고도, fix, 위성 카운트
- 배터리: `bat_mv`
- 내부 온도: DS18B20 (`temp_c_x100`)
- 공기질: PMS3003 (`pm1/pm2.5/pm10`)
- 외기 온습도: SHT31-D (`sht31_temp_c_x100`, `sht31_rh_x100`)
- 기압/온도/고도: MS5611 (`ms5611_press_pa`, `ms5611_temp_c_x100`, `ms5611_alt_m`)

추가로 payload의 일부 예약 필드는 런타임 관측/상태 플래그 용도로 사용됩니다(프로토콜 호환성 유지 목적).
- `reserved1`: RealTime 루프 실행시간(us, saturate)
- `reserved2/reserved3`: health 확장 플래그(하위/상위 바이트)
- `reserved4`: telemetry tick 실행시간(100us 단위, 0..255 => 0..25.5ms)

> 위 의미는 펌웨어 디버그/튜닝을 위해 사용 중이며, ESP32 수신 측과 함께 변경/고정하는 것을 권장합니다.

## FreeRTOS 태스크 구조(요약)

우선순위 기반 선점형 스케줄링으로, 경로별 “최악 지연”이 50Hz를 깨지 않도록 분리합니다.

- RealTime(최고 우선순위): 20ms 주기(=50Hz), 텔레메트리용 스냅샷 갱신/핵심 경로
- Sensor: 100ms 기반(센서/서비스 폴링 및 분배)
- System: 1000ms(상태 모니터링/하우스키핑)

공유 데이터(텔레메트리 payload)는 mutex로 보호하고, RealTime 측은 **0-timeout 스냅샷 읽기(try-lock)**로 지터를 억제합니다.

## 현재 포함된 센서/서비스

- GPS (XA1110 / USART1): NMEA 파서 + 서비스
- PMS3003 (USART2): 스트리밍 프레임 파서 + 서비스
- DS18B20 (1-Wire GPIO): 드라이버 + 1Hz 서비스
- Battery ADC (ADC1_IN2): 드라이버 + 1Hz 서비스
- SHT31-D (I2C3): 드라이버 + 1Hz 서비스
- MS5611 (I2C3): 드라이버 + 약 10Hz 서비스

> 참고: 저장소에는 이 외에도 다양한 센서/서비스 모듈이 포함되어 있습니다. 최신 목록은 `Core/Inc/services/` / `Core/Inc/drivers/`를 기준으로 확인하세요.

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

MS5611 변환 품질/속도 트레이드오프는 OSR로 조정할 수 있습니다.
- 기본값(고품질): `MS5611_OSR_4096`
- 기본 빌드는 `platformio.ini`에서 `-DMS5611_SERVICE_OSR=MS5611_OSR_4096`로 4096을 명시합니다.
- 더 빠른 업데이트가 필요하면 빌드 플래그로 변경:
  - `-DMS5611_SERVICE_OSR=MS5611_OSR_2048`
  - `-DMS5611_SERVICE_OSR=MS5611_OSR_1024`

#### 실기기 검증 체크리스트(OSR=4096 기준)

목표는 “텔레메트리 50Hz 유지”를 최우선으로 하면서, MS5611이 **체감 20Hz-class**로 충분히 갱신되는지 확인하는 것입니다.

- 텔레메트리 수신 측(ESP32/호스트)에서 프레임 주기가 20ms(=50Hz)로 안정적인지 확인
- payload 예약 필드로 타이밍 여유 확인
  - `reserved1`(RealTime 루프 실행시간 us): 평균/최댓값이 20ms 대비 충분히 낮은지(스파이크 유무)
  - `reserved4`(telemetry tick 실행시간 100us 단위): 0..255 범위에서 포화(255)나 급증이 없는지
- MS5611 데이터가 “20Hz-class”로 갱신되는지 확인
  - 연속 프레임에서 `ms5611_press_pa`, `ms5611_temp_c_x100`가 자주(대략 2~3프레임마다) 업데이트되는지
  - 동일 값이 오래 반복되거나, 갱신이 뚝 끊기는 구간이 있는지
- I2C 오류/리커버리 빈도 확인
  - 기압/온습도 계열 값이 간헐적으로 0/고정값으로 떨어지면 I2C timeout/recovery가 자주 발생하는 신호일 수 있음

문제가 있으면 다음 순서로만 조정하는 것을 권장합니다.
- 우선 OSR를 낮춰 변환시간을 줄이기: `-DMS5611_SERVICE_OSR=MS5611_OSR_2048` → `MS5611_OSR_1024`
- 그래도 부족하면 Sensor 태스크 주기/슬롯(센서 폴링) 측면을 재조정

## 개발 가이드

- 새로운 센서 추가는 “드라이버 + 서비스 + 텔레메트리 필드 + (가능하면) native 유닛 테스트”를 한 세트로 추가하는 방식이 가장 안전합니다.
- HAL 의존 부분은 `native_test`에 넣지 말고, 순수 로직(프레이밍/파서/보정/CRC)은 codec 모듈로 분리해 테스트하세요.
- 메모리 제약: Flash/RAM이 작기 때문에 큰 버퍼/정적 데이터 추가에 주의하세요.

## ESP32 수신 로깅(권장)

50Hz 안정성(지터/드롭)을 빠르게 확인하려면, ESP32 수신 측에서 **프레임을 “정상 파싱+CRC OK”로 확정한 시점**에 아래 CSV 한 줄을 찍는 방식을 권장합니다.

권장 컬럼(예시):
- `rx_t_us`: ESP32 로컬 단조 증가 타임스탬프(us) (`esp_timer_get_time()` 또는 Arduino `micros()`)
- `seq`: 프레임 시퀀스(`telemetry_frame.h` 헤더의 `seq`)
- `crc_ok`: 1(OK) / 0(FAIL)
- `frame_ts_ms`: 프레임 헤더의 `timestamp_ms`(STM32 `HAL_GetTick()` 기반)
- `reserved1_us`: payload `reserved1`(RealTime 루프 실행시간 us 용도)
- `reserved4_100us`: payload `reserved4`(telemetry tick 실행시간 100us 단위)

이 CSV는 아래 도구로 요약/히스토그램을 바로 확인할 수 있습니다.
- `python tools/telemetry_rx_log_analyze.py --csv your_log.csv --expected-period-ms 20 --only-ok`

Arduino(ESP32) 기준 수신/로깅 예제 스케치는 다음 파일을 참고하세요:
- `docs/telemetry_rx_arduino_example.ino`

출력 예시(CSV):
```
rx_t_us,seq,crc_ok,frame_ts_ms,reserved1_us,reserved4_100us
123456789,42,1,98765,3200,18
123476810,43,1,98785,3100,19
...
```

## 오프라인 타이밍/데드라인 분석(tools)

실기기 없이도 “50Hz가 깨질 위험”을 빠르게 탐색하기 위해 호스트용 스크립트를 제공합니다.

- 단일 시뮬(가정한 WCET로 데드라인 미스 여부 확인)
  - `python tools/rtos_deadline_sim.py`
- RealTime WCET 스윕(임계값 탐색)
  - `python tools/rtos_deadline_sim.py --sweep-rt-wcet --sim-ms 5000 --sweep-start-us 8000 --sweep-end-us 20000 --sweep-step-us 1000`
- 그래프(PNG) 원클릭 생성(스윕→CSV/SVG→PNG)
  - `python tools/rtos_sweep_plot_matplotlib.py --run-sweep`

자세한 사용법은 `tools/README.md`를 참고하세요.

---

### 참고

- 자세한 작업 규칙/컨벤션은 `.github/copilot-instructions.md`에 정리되어 있습니다.
