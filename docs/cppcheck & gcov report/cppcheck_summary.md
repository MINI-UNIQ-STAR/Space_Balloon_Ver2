# Cppcheck 정적 분석 결과 요약

## 분석 개요
- **날짜**: 2026-01-23
- **도구**: Cppcheck 2.x
- **대상**: Core/Src (26 파일)

## 결과 요약

| 카테고리 | 개수 | 설명 |
|---------|------|------|
| **Error** | 0 | ❌ 치명적 오류 없음 |
| **Warning** | 0 | ⚠️ 경고 없음 |
| **Style** | ~50 | 코딩 스타일 개선 권장 |
| **Information** | ~70 | 헤더 경로 (무시 가능) |

## 주요 발견 사항

### 1. const 포인터 권장 (Style)
```
bsp.c: Parameter 'pData' can be declared as pointer to const
```
→ 수정 불필요, 안전성 향상 목적

### 2. 미사용 변수 (Style)
```
actuators.c:60: Variable 'ccr_val' is assigned but never used
```
→ UNIT_TEST 모드에서만 발생, 무시 가능

### 3. 미사용 함수 (Style) - HAL 콜백
```
HAL_ADC_MspInit, HAL_UART_MspInit, IRQ Handlers...
```
→ **정상**: 이 함수들은 HAL 라이브러리가 내부적으로 호출함

### 4. static 권장 함수
```
FDIR_ReportFailure, Sensors_Init_I2C1, CRC16_CCITT...
```
→ 향후 외부 호출 가능성 있어 현재 상태 유지

## 결론
**치명적 오류(Error) 0건** - 코드가 정적 분석 관점에서 안전합니다.
발견된 항목은 모두 스타일 권장사항이며, 기능에 영향 없습니다.
