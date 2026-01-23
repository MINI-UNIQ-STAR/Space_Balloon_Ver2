# xcp.c XCP 프로토콜 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Calibration |
| **코드 라인** | 152줄 |
| **역할** | XCP 프로토콜 구현 |
| **최종 업데이트** | 2026-01-18 |

## 구조

| 함수 | 설명 |
|------|------|
| `XCP_Init()` | 버퍼 초기화 |
| `XCP_ProcessCommand()` | 명령어 처리 (Connect/Upload/Download) |
| `XCP_SendResponse()` | 응답 전송 |
| `XCP_UpdateMeasurements()` | DAQ 업데이트 |

## 지원 명령어

| 명령 | 코드 | 설명 |
|------|------|------|
| `CC_CONNECT` | 0xFF | 연결 수립 |
| `CC_SHORT_UPLOAD` | 0xF4 | 메모리 읽기 |
| `CC_SHORT_DOWNLOAD` | 0xF0 | 메모리 쓰기 |
| `CC_GET_STATUS` | 0xFD | 상태 조회 |

## 상태 머신
```
DISCONNECTED → (Connect) → CONNECTED
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **완성도** | ⭐⭐⭐⭐⭐ | 핵심 기능 구현 |
| **확장성** | ⭐⭐⭐⭐⭐ | DAQ 확장 가능 |
| **안전성** | ⭐⭐⭐⭐ | NULL 포인터 검사 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**ASAM XCP 표준을 준수하는 Connect/Upload/Download 명령을 충실히 구현함. 이를 통해 비행 중 파라미터 튜닝 및 실시간 데이터 모니터링을 위한 강력한 인터페이스를 제공함.**
