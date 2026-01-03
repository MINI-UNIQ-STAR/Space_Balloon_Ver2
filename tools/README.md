# tools

이 폴더는 **실기기 없이도** FreeRTOS 태스크 스케줄링/데드라인 리스크를 빠르게 확인하고, 결과를 **CSV/SVG/PNG**로 뽑아볼 수 있는 호스트용 스크립트들을 모아둡니다.

> 주의: 여기 스크립트들은 “정확한 MCU 사이클-정밀 시뮬레이터”가 아니라, **가정한 WCET(최악 실행시간)** 과 간단한 ISR 부하 모델을 넣고 **데드라인 미스 가능성**을 보는 *근사 모델*입니다.

## 빠른 시작

가상환경이 활성화되어 있다면(권장):

- 단일 실행(현재 WCET 가정으로 1회 시뮬레이션)
  - `python tools/rtos_deadline_sim.py`

- 50Hz(20ms) 기준으로 RT WCET 스윕 표 출력
  - `python tools/rtos_deadline_sim.py --sweep-rt-wcet --sim-ms 5000 --sweep-start-us 8000 --sweep-end-us 20000 --sweep-step-us 1000`

- 스윕 결과를 그래프로(원클릭: 스윕→CSV/SVG→PNG)
  - `python tools/rtos_sweep_plot_matplotlib.py --run-sweep`

결과 파일은 기본적으로 아래에 생성됩니다.
- `tools/out/rtos_wcet_vs_isr.csv`
- `tools/out/rtos_wcet_vs_isr.svg`
- `tools/out/rtos_wcet_vs_isr.png`

## 스크립트 목록

### 1) `rtos_deadline_sim.py`

고정 우선순위 선점형(FreeRTOS-like) 스케줄러를 **100us 단위(기본)**로 근사 시뮬레이션하여, 각 태스크의 데드라인 미스 여부를 출력합니다.

- 기본 가정 태스크
  - `RealTime`: 20ms period / 20ms deadline
  - `Sensor`: 100ms period / 100ms deadline
  - `System`: 1000ms period / 1000ms deadline

- 주요 옵션
  - `--sim-ms`: 시뮬레이션 시간(ms)
  - `--quantum-us`: 시간 퀀텀(us). `1000 % quantum == 0` 이어야 함
  - `--rt-wcet-us`, `--sensor-wcet-us`, `--system-wcet-us`: 각 태스크 WCET(us)
  - `--isr-tax-period-ms`, `--isr-tax-us`: 단순 ISR 부하 모델(예: 1ms마다 100us = 약 10% CPU)

#### RT WCET 스윕 모드

`--sweep-rt-wcet`를 주면 RT WCET 범위에 대해 표를 출력하고, 다음 임계값을 요약합니다.

- `RealTime misses=0` 최대 RT WCET
- `RealTime+Sensor+System misses=0` 최대 RT WCET

예시:
- `python tools/rtos_deadline_sim.py --sweep-rt-wcet --sim-ms 5000`
- `python tools/rtos_deadline_sim.py --sweep-rt-wcet --sim-ms 5000 --isr-tax-period-ms 1 --isr-tax-us 100`

### 2) `rtos_sweep_plot.py`

외부 의존성 없이(=matplotlib 없이) `rtos_deadline_sim`를 호출하여 ISR tax에 따른 “허용 가능한 RT WCET”을 계산하고,
- CSV
- SVG

를 생성합니다.

- 실행
  - `python tools/rtos_sweep_plot.py`

- 주요 옵션
  - `--isr-tax-us-list`: `0,50,100,...` 형식(기본 period=1ms)
  - `--out-dir`: 출력 폴더(기본 `tools/out`)

### 3) `rtos_sweep_plot_matplotlib.py`

matplotlib을 사용해 `tools/out/rtos_wcet_vs_isr.csv`를 읽고 PNG 이미지를 생성합니다.

- PNG만 렌더(기존 CSV가 있을 때)
  - `python tools/rtos_sweep_plot_matplotlib.py`

- 원클릭(스윕→CSV/SVG 갱신→PNG)
  - `python tools/rtos_sweep_plot_matplotlib.py --run-sweep`

- 출력
  - `tools/out/rtos_wcet_vs_isr.png`

## 해석 가이드(권장)

- `RealTime misses=0`만 만족하는 구간은 **다른 태스크가 굶을 수** 있습니다.
  - 실제 시스템 안정성 관점에서는 보통 `All tasks misses=0`(RT+Sensor+System) 기준 임계값을 우선으로 봅니다.
- ISR tax를 높일수록(인터럽트/드라이버 부하가 클수록) RT 여유가 줄어듭니다.
  - 실기기에서 관측한 worst-case 스파이크가 있으면, 그 수치를 `--rt-wcet-us`로 보수적으로 반영하세요.

## 트러블슈팅

- `ModuleNotFoundError: No module named 'matplotlib'`
  - matplotlib 기반 PNG가 필요하면 설치 후 실행하세요:
    - `python -m pip install matplotlib`

- 결과 파일이 안 보임
  - 기본 출력 경로는 `tools/out/` 입니다.

## 파일 생성물

- `tools/out/` 폴더는 스크립트 실행 결과(CSV/SVG/PNG)를 저장합니다.
  - 커밋 정책은 프로젝트 컨벤션에 맞춰서 결정하세요(필요 시 `.gitignore`로 제외).

---

## (추가) ESP32 수신 로그 분석

실기기에서 ESP32가 수신한 텔레메트리 프레임 간격(지터/드롭)을 빠르게 확인하기 위한 분석 도구입니다.

### `telemetry_rx_log_analyze.py`

- 입력: ESP32 수신 측에서 찍은 CSV(헤더 포함)
- 출력: 프레임 간격(dt) 통계(평균/최댓값/p99 등), 시퀀스 갭 기반 드롭 추정, 간단 히스토그램

실행 예시:
- `python tools/telemetry_rx_log_analyze.py --csv your_log.csv --expected-period-ms 20 --only-ok`

Arduino(ESP32) 수신/CSV 로깅 예제 스케치:
- `docs/telemetry_rx_arduino_example.ino`
