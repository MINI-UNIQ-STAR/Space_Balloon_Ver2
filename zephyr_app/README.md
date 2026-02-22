# Space Balloon Ver2 - Zephyr RTOS Port

## 프로젝트 구조

```
zephyr_app/
├── CMakeLists.txt          # Zephyr 빌드 설정
├── prj.conf                 # Kconfig 설정
├── boards/                  # 커스텀 보드 정의
│   └── stm32g431cb/        # STM32G431CBU6 보드
├── src/
│   ├── main.c              # 메인 진입점
│   ├── fdir.c              # FDIR 모듈 (포팅)
│   ├── sensors.c           # 센서 관리 (포팅)
│   ├── telemetry.c         # 텔레메트리 (포팅)
│   ├── kalman.c            # 칼만 필터 (포팅)
│   ├── pid.c               # PID 제어 (포팅)
│   └── actuators.c         # 액추에이터 (포팅)
├── include/
│   ├── fdir.h
│   ├── sensors.h
│   └── ...
└── tests/                   # 단위 테스트
```

## 빌드 타겟

| 타겟 | 용도 |
|------|------|
| `qemu_cortex_m3` | 시뮬레이션 (개발/테스트) |
| `stm32g431cb` | 실제 하드웨어 |

## 빌드 명령

```bash
# QEMU 시뮬레이션
west build -b qemu_cortex_m3 zephyr_app
west build -t run

# 실제 하드웨어
west build -b stm32g431cb zephyr_app
west flash
```

## 포팅 진행률

- [x] 기본 Zephyr 프로젝트 구조
- [ ] FDIR 모듈 포팅
- [ ] 센서 드라이버 포팅
- [ ] 텔레메트리 포팅
- [ ] QEMU 테스트
- [ ] 실제 하드웨어 테스트
