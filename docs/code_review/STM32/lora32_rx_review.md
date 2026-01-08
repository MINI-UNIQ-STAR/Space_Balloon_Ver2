# telemetry_rx_lora32v2.1.ino 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | LoRa32 v2.1 (ESP32 + SX1276) |
| **코드 라인** | 469줄 |
| **역할** | STM32 텔레메트리 수신/저장/전송 |

---

## 기능 아키텍처

```
STM32  ─UART(50Hz)─►  LoRa32  ─┬─SD(1Hz)─►  CSV 로그
                                └─LoRa(5초)─►  지상국
```

---

## 핵심 기능

| 기능 | 주기 | 설명 |
|------|------|------|
| **UART 수신** | 50Hz (20ms) | STM32 텔레메트리 프레임 |
| **SD 저장** | 1Hz | 실제값 CSV 변환 |
| **LoRa 전송** | 0.2Hz (5초) | Raw 바이너리 전달 |

---

## 구조 분석

### 페이로드 구조체 (Line 45-109)
```c
#pragma pack(push, 1)
typedef struct { ... } telemetry_payload_t; // ~118 bytes
typedef struct { ... } telemetry_frame_t;   // Header + Payload + CRC
#pragma pack(pop)
```
✅ STM32 `telemetry.h`와 완전 동기화

### 프레임 파서 (Line 414-455)
```c
// Magic byte 동기화: 0xA5 0x5A
if (rxIdx == 0) { if (b == 0xA5) rxFrame[rxIdx++] = b; }
if (rxIdx == 1) { if (b == 0x5A) rxFrame[rxIdx++] = b; }
// 페이로드 길이 파싱 후 전체 프레임 수신
```
✅ 바이트 단위 스트리밍 파서 (ISR 친화적)

### CRC 검증 (Line 147-156)
```c
uint16_t crc16_ccitt_false(data, len) // Poly 0x1021, Init 0xFFFF
```
✅ STM32측과 동일한 알고리즘

---

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **프로토콜 동기화** | ⭐⭐⭐⭐⭐ | telemetry.h 미러링 |
| **에러 처리** | ⭐⭐⭐⭐⭐ | CRC, overflow, magic |
| **데시메이션** | ⭐⭐⭐⭐⭐ | 50Hz→1Hz(SD), 5s(LoRa) |
| **CSV 변환** | ⭐⭐⭐⭐⭐ | ISO 8601 GPS 시간 |
| **버퍼 관리** | ⭐⭐⭐⭐⭐ | 60프레임 버퍼, 1KB UART |

---

## 장점
- ✅ **완전한 프레임 파싱** (Magic → Header → Payload → CRC)
- ✅ **GPS UTC 시간** ISO 8601 포맷 (`YYYY-MM-DDTHH:MM:SS`)
- ✅ **x100/x1000 스케일** 자동 변환
- ✅ **LoRa SF11** 장거리 설정
- ✅ **UART 버퍼 확장** (1KB, SD 쓰기 중 손실 방지)

## 개선 권장
- ⚠️ LoRa 채널 충돌 검사 없음 (CSMA 권장)
- ⚠️ SD 파일 로테이션 없음 (용량 초과 시)

---

## LoRa 설정
```c
LORA_FREQ = 915MHz
SF = 11, BW = 125kHz, CR = 4/5
```
예상 Data Rate: ~537 bps

---

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**프로덕션 레디 텔레메트리 수신기.**  
STM32-LoRa32 통합 완벽. CSV 로깅 + LoRa 중계 동시 수행.
