# XA1110 GPS 드라이버 코드 리뷰

| 항목 | 내용 |
|------|------|
| **센서** | XA1110 Multi-GNSS 모듈 |
| **인터페이스** | UART (NMEA) |
| **코드 라인** | ~115줄 |

## 구조

| 함수 | 설명 |
|------|------|
| `XA1110_Init()` | Balloon Mode + 10Hz 설정 |
| `XA1110_ProcessByte()` | 바이트 수신기 |
| `XA1110_ParseSentence()` | NMEA 파싱 |

## 지원 문장

| 문장 | 데이터 |
|------|------|
| RMC | 위치, 시간, 날짜 |
| GGA | Fix, 고도, 위성 수 |
| GSA | 2D/3D Fix |
| **GSV** | **시스템별 위성 수** |

## Multi-GNSS 지원 (신규)

```c
// Talker ID별 위성 수 파싱
GP → ctx->data.sats_gps
GL → ctx->data.sats_glonass
GA → ctx->data.sats_galileo
GB → ctx->data.sats_beidou
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **Balloon Mode** | ⭐⭐⭐⭐⭐ | 고도 >18km 지원 |
| **Multi-GNSS** | ⭐⭐⭐⭐⭐ | 4개 시스템 |
| **ISR 친화** | ⭐⭐⭐⭐⭐ | 바이트 파서 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**성층권 비행 최적화 GPS 드라이버.**
