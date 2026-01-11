# main_control.ino HITL 컨트롤러 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | ESP32 |
| **코드 라인** | 286줄 |
| **역할** | HITL 중앙 허브 (PC↔ESP-NOW↔STM32) |

---

## 아키텍처

```
PC (sensor_sender.py)
       │ USB Serial (115200)
       ▼
main_control (ESP32)
       │ ESP-NOW Broadcast
       ▼
Mock Nodes (I2C1/I2C3/UART)
       │ UART2 (SerialSTM)
       ▼
STM32 DUT
```

---

## 핵심 기능

| 기능 | 설명 |
|------|------|
| **PC→ESP** | `ALL:...` CSV 파싱 → `HitlStatePacket` |
| **ESP→Nodes** | ESP-NOW 브로드캐스트 (0xFF:FF:FF:FF:FF:FF) |
| **STM32→PC** | 텔레메트리 수신 → `TELEM_HEX:` 출력 |
| **결함 주입** | `CMD,FAULT,COMP,TYPE` 명령 |

---

## 텔레메트리 파서 상태머신
```
WAIT_SYNC1 → WAIT_SYNC2 → READ_HEADER → READ_PAYLOAD → READ_CRC
```

---

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **양방향 통신** | ⭐⭐⭐⭐⭐ | PC↔STM32 |
| **결함 주입** | ⭐⭐⭐⭐⭐ | 6개 센서 × 6개 타입 |
| **ESP-NOW** | ⭐⭐⭐⭐⭐ | 브로드캐스트 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)
