# stm32_spaceballoon (STM32G431 + CubeMX/HAL + FreeRTOS + PlatformIO)

## Source of truth / 편집 규칙
- CubeMX가 source of truth: `stm32_spaceballoon.ioc` → `Core/` 생성. 핀/AF/NVIC/baud 등은 `.ioc` + 생성 코드(`Core/Src/*`)를 기준으로 확인.
- 시스템 요구사항/설계 의도(spec)는 `docs/STM32_SpaceBalloon_Specification.md`(Rev 3.0)를 기준으로 하되, **구현 세부(핀/AF/NVIC/baud/핸들 연결)가 충돌하면** `.ioc` + 생성 코드가 최종 기준.
- `Core/` 내 생성 파일은 `/* USER CODE BEGIN/END */` 블록만 수정. 새 기능은 가능하면 새 모듈로 `Core/Src/{app,services,drivers}/` + `Core/Inc/...`에 추가.
- 실제 소스/헤더: `Core/Src/`, `Core/Inc/` (`src/`는 문서용, `archive/`는 건드리지 않음).

## Big picture (왜 이렇게 설계됐나)
- 목표: 센서 지연/리커버리가 있어도 **USART3로 50Hz 텔레메트리 프레임**을 안정적으로 송신.
- “센서 읽기(느림)”와 “송신(20ms 주기)”를 분리: 텔레메트리는 스냅샷 payload를 읽어 프레임 전송.
- 텔레메트리 프레이밍/CRC: `Core/Inc/services/telemetry_frame.h`, 구현/송신 경로는 `Core/Src/services/telemetry_service.c` 중심.

## 레이어/의존성 규칙
- 단방향: `app` → `services` → `drivers` → `HAL` (상위에서 HAL 직접 호출은 피하고 driver로 감싸기).
- 순수 로직(파서/CRC/보정수식)은 `*_codec.*`로 분리해 호스트 테스트 가능하게 유지.

## 빌드/디버그/테스트 (PlatformIO)
- 기본 빌드: `pio run` (env: `genericSTM32G431CB`), 업로드: `pio run -t upload`, 모니터: `pio device monitor`.
- 지상 디버그용 env: `genericSTM32G431CB_debug` (SWD probe용 플래그 활성).
- 호스트 유닛테스트(Unity): `pio test -e native_test` (`platformio.ini`의 `build_src_filter`에 포함된 codec/프레임/파서 위주).

## 버스/포트 매핑은 “코드 기준”
- 문서가 상충할 수 있어(예: 사양서 vs README), 실제 사용 버스는 드라이버에서 확인: `extern I2C_HandleTypeDef hi2c*;`.
- 예: `Core/Src/drivers/sht31.c`, `Core/Src/drivers/ms5611.c`는 `hi2c3` 사용.
- UART 설정/baud는 `Core/Src/usart.c`를 기준으로 확인(역할: USART1=GPS, USART2=PMS, USART3=Telemetry).

## 기타 주의
- `Core/Src/syscalls.c`의 `_write()` → `__io_putchar()` 라우팅 때문에 `printf` 출력 UART를 신중히 선택(역할 충돌 주의).
- 링크/메모리: `STM32G431CBUX_FLASH.ld` (소형 RAM/Flash이므로 큰 static 버퍼 주의).
- 수정 금지: `Drivers/`, 자동 생성 `.vscode/*`, `archive/`.