# sensor_sender.py HITL 대시보드 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | Python 3 + PySide6 |
| **코드 라인** | 971줄 |
| **역할** | HITL 시뮬레이션 GUI 대시보드 |

---

## 기능 아키텍처

```
sensor_sender.py
├── SensorSenderGUI (QMainWindow)
│   ├── COM 연결/단절
│   ├── 시뮬레이션 모드 (General/Scenario/FDIR)
│   ├── 그래프 업데이트
│   └── 결함 주입 메뉴
├── TelemetryReceiver (Thread)
│   └── STM32 텔레메트리 CSV 로깅
└── sim_core (Rust Optional)
    └── 고성능 시뮬레이션 (50Hz)
```

---

## 핵심 기능

| 기능 | 설명 |
|------|------|
| **시리얼 통신** | 115200 bps, "ALL:..." 프로토콜 |
| **GUI 대시보드** | 4개 그래프 (Map/Alt/IMU/Payload) |
| **CSV 로깅** | 텔레메트리 수신 저장 |
| **결함 주입** | GPS/IMU/BARO 타임아웃/노이즈 |
| **Rust 가속** | `sim_core` 모듈 (선택적) |

---

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **UI 구조** | ⭐⭐⭐⭐⭐ | PySide6 Dark Theme |
| **프로토콜** | ⭐⭐⭐⭐⭐ | telemetry.h 동기화 |
| **스레드** | ⭐⭐⭐⭐⭐ | RX 분리 |
| **확장성** | ⭐⭐⭐⭐⭐ | Rust 옵션 |

---

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**HITL 통합 테스트 골든 레퍼런스.**
