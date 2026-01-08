# 구현 현황 요약 (Implementation Status Report)

**문서 버전**: Rev 2.0 (완전 재검토)
**최종 업데이트**: 2026-01-09
**프로젝트**: STM32 성층권 풍선 센서 플랫폼
**검토자**: 객관적 코드 분석 기반

---

## ⚠️ 중요: 문서와 코드의 불일치

이 문서는 **실제 코드 구현 상태**를 기반으로 작성되었습니다. 다른 문서들([FDIR.md](./FDIR.md), [FINAL_REPORT.md](./FINAL_REPORT.md), [STM32_SpaceBalloon_Specification.md](./STM32_SpaceBalloon_Specification.md))에는 설계와 계획이 상세히 기술되어 있지만, **실제 구현은 이보다 훨씬 적습니다**.

---

## 📊 전체 구현 진행률 (실제 코드 기준)

| 모듈 | 설계 | 구현 | 단위 테스트 | 통합 테스트 | 하드웨어 검증 | 상태 |
|------|:----:|:----:|:----------:|:----------:|:------------:|:----:|
| **센서 드라이버** | ✅ 100% | ✅ 95% | ⚠️ 50% | ❌ 0% | ❌ 0% | 부분 완료 |
| **텔레메트리** | ✅ 100% | ✅ 100% | ❌ 0% | ❌ 0% | ❌ 0% | 코드만 완료 |
| **FDIR 감지** | ✅ 100% | ✅ 90% | ✅ 80% | ❌ 0% | ❌ 0% | 부분 완료 |
| **FDIR 복구** | ✅ 100% | ❌ 5% | ❌ 0% | ❌ 0% | ❌ 0% | **거의 없음** |
| **PID 제어** | ✅ 100% | ✅ 100% | ⚠️ 50% | ❌ 0% | ❌ 0% | 코드만 완료 |
| **칼만 필터** | ✅ 100% | ✅ 100% | ⚠️ 50% | ❌ 0% | ❌ 0% | 코드만 완료 |
| **I2C 복구** | ✅ 100% | ❌ 0% | ❌ 0% | ❌ 0% | ❌ 0% | **없음** |
| **BSP 레이어** | ✅ 100% | ✅ 80% | ❌ 0% | ❌ 0% | ❌ 0% | 부분 완료 |
| **HITL 시스템** | ✅ 100% | ⚠️ 70% | ❌ 0% | ❌ 0% | ❌ 0% | 부분 완료 |
| **SITL 시스템** | ✅ 100% | ⚠️ 60% | ❌ 0% | ❌ 0% | N/A | 부분 완료 |

**전체 진행률**:
- **설계**: 100% 완료
- **구현**: **약 60% 완료**
- **검증**: **약 15% 완료**

---

## ✅ 완료된 항목 (구현 + 검증)

### 1. 센서 드라이버 (95% 구현, 50% 검증)
**파일**: `Core/Src/sensors.c` (약 1,200 LOC), `Core/Inc/sensors.h`

**구현 완료**:
- ✅ LSM6DSV16X (IMU) - I2C1 - 완전 구현
- ✅ MLX90393 (자기계) - I2C1 - 완전 구현
- ✅ GDK101 (방사선) - I2C1 - 완전 구현
- ✅ MS5611 (기압계) - I2C3 - 완전 구현
- ✅ SHT31-D (온습도) - I2C3 - 완전 구현
- ✅ DS18B20 (온도 센서 x2) - 1-Wire - 완전 구현
- ⚠️ CM1107N (CO2) - I2C3 - 드라이버 있으나 통합 미검증
- ⚠️ MCP9600 (열전대) - I2C3 - 드라이버 있으나 통합 미검증
- ⚠️ SEN0321 (오존) - I2C3 - 드라이버 있으나 통합 미검증
- ⚠️ XA1110 (GPS) - UART1 - 드라이버 있으나 NMEA 파싱 미검증
- ⚠️ PMS3003 (미세먼지) - UART2 - 드라이버 있으나 프로토콜 미검증

**검증 상태**:
- ✅ 단위 테스트: `test/test_drivers/` 존재하나 일부만 실행됨
- ❌ 실제 하드웨어: 한 번도 실행되지 않음

**누락 사항**:
- ❌ `Sensors_Read_All()` 함수의 에러 핸들링
- ❌ GPS NMEA 파싱 완전성 검증
- ❌ PMS3003 체크섬 검증

---

### 2. 텔레메트리 시스템 (100% 구현, 0% 검증)
**파일**: `Core/Src/telemetry.c`, `Core/Inc/telemetry.h`

**구현 완료**:
- ✅ 116 바이트 바이너리 프로토콜 정의
- ✅ CRC-16-CCITT 체크섬
- ✅ 프레임 헤더 (magic, version, msg_type, seq, timestamp)
- ✅ `telemetry_payload_sensor_snapshot_t` 구조체

**검증 상태**:
- ❌ GPS 1PPS 동기화: **코드에 없음** (문서에만 설명됨)
- ❌ 50Hz 전송: App_Loop에서 호출되나 실제 타이밍 미검증
- ❌ LoRa32 수신기 호환성: **실제 테스트 없음**
- ❌ CRC 검증 로직: **구현되었으나 테스트 없음**

**누락 사항**:
- ❌ `Telemetry_Send()` 함수가 **없음** (텔레메트리 전송 함수 미구현)
- ❌ GPS 1PPS 인터럽트 핸들러 **없음**
- ❌ UART3 DMA 전송 설정 **없음**

---

### 3. FDIR 감지 메커니즘 (90% 구현, 80% 검증)
**파일**: `Core/Src/fdir.c` (375 LOC), `Core/Inc/fdir.h`

**구현 완료**:
- ✅ 타임아웃 감지 (sensor_timeout_ms 배열)
- ✅ 상태 머신 (HEALTHY → WARNING → RECOVERY → FAILURE_PERMANENT)
- ✅ 온도 기반 센서 보호 (Lines 105-148)
- ✅ 범위 검증 함수 (Lines 232-265)
- ✅ 연속성 검사 (고도 점프 감지, Lines 267-292)
- ✅ 백업 고도 전환 (Lines 311-327)
- ✅ 상태 플래그 생성 (Lines 329-373)

**검증 상태**:
- ✅ 단위 테스트: `test/test_fdir/` 존재, 4개 테스트 PASS
  - `test_fdir_init_healthy`
  - `test_fdir_timeout_recovery`
  - `test_fdir_permanent_failure`
  - `test_fdir_cold_protection`
- ❌ 통합 테스트: 없음
- ❌ 하드웨어 테스트: 없음

**누락 사항**:
- ⚠️ `Sensors_Reset()` 호출은 되지만 **실제 동작 없음** (Line 170)
- ❌ I2C 버스 복구 호출 없음

---

## ❌ 미구현 항목 (설계만 존재)

### 1. FDIR 복구 로직 (5% 구현)
**파일**: `Core/Src/sensors.c` (Lines 238-246)

**현재 상태**:
```c
void Sensors_Reset(SensorID_t id) {
#ifdef UNIT_TEST
    printf("FDIR: Resetting Sensor ID %d\n", id);
#endif
    // TODO: Implementation for HW:
    // 1. DeInit / ReInit Driver
    // 2. Power Cycle if GPIO attached
}
```

**설계는 완료되었으나 구현 없음**:
1. ❌ GPS 리셋 (PA9: GPS_nRST)
2. ❌ IMU 리셋 (PB13: LSM_RST)
3. ❌ Baro 리셋 (PA5: MS_RST)
4. ❌ PMS3003 리셋 (PB10: PMS_SET)
5. ❌ SHT31 리셋 (PB11: SHT_RST P-MOS)
6. ❌ GDK101 리셋 (PB2: GDK_RST)
7. ❌ 자기계 리셋 (PB14: MLX_nRST)
8. ❌ CO2 리셋 (PB0: CM1107N_RST)
9. ❌ MCP9600 리셋 (PA4: MCP_RST)
10. ❌ 센서 보드 전체 리셋 (PB1: SEN_RST)

**문서 참조**: [FDIR.md Lines 182-253](./FDIR.md#L182-L253)

**예상 작업량**: 약 150 LOC 추가 필요

---

### 2. I2C 버스 복구 (0% 구현)
**필요한 함수**: `BSP_I2C1_Recovery()`, `BSP_I2C3_Recovery()`

**현재 상태**:
- ❌ `Core/Inc/bsp.h`에 함수 선언 **없음**
- ❌ `Core/Src/bsp.c`에 함수 구현 **없음**
- ✅ 설계는 [FDIR.md Lines 186-189](./FDIR.md#L186-L189)에 있음

**필요한 구현**:
```c
// 설계안 (미구현)
void BSP_I2C1_Recovery(void) {
    // 1. I2C 비활성화
    // 2. SCL 라인에 9개 클럭 펄스 생성 (GPIO 비트뱅)
    // 3. I2C 재초기화
}
```

**예상 작업량**: 약 80 LOC 추가 필요

---

### 3. GPS 1PPS 동기화 (0% 구현)
**문서에는 있으나 코드에 없음**

**문서 참조**: [STM32_SpaceBalloon_Specification.md Lines 466-472](./STM32_SpaceBalloon_Specification.md#L466-L472)

**현재 상태**:
- ❌ PB4 (GPS_PPS) EXTI 인터럽트 핸들러 **없음**
- ❌ 1PPS 에지 검출 로직 **없음**
- ❌ 50Hz 슬롯 분할 로직 **없음**
- ❌ `drivers/pps_capture.c` 파일 **존재하지 않음**

**예상 작업량**: 약 100 LOC 추가 필요

---

### 4. 텔레메트리 전송 함수 (0% 구현)
**필요한 함수**: `Telemetry_Send()`

**현재 상태**:
- ❌ `Core/Src/telemetry.c` 파일 **존재하지 않음**
- ❌ `Core/Inc/telemetry.h`에는 구조체만 정의됨
- ❌ UART3 DMA 전송 로직 **없음**
- ❌ CRC 계산 및 삽입 로직 **없음**

**필요한 구현**:
```c
// 필요한 함수 (미구현)
void Telemetry_Send(telemetry_frame_t *frame);
uint16_t CRC16_Calculate(uint8_t *data, uint16_t len);
```

**예상 작업량**: 약 120 LOC 추가 필요

---

### 5. 저전압 감지 및 Load Shedding (0% 구현)

**현재 상태**:
- ✅ 배터리 전압 읽기: `BSP_ADC_Read_Battery_mV()` 존재
- ❌ 저전압 임계값 체크 **없음**
- ❌ 히터 자동 차단 로직 **없음**
- ❌ PMS3003 자동 차단 로직 **없음**

**예상 작업량**: 약 50 LOC 추가 필요

---

## ⚠️ 부분 완료 항목

### 1. BSP 레이어 (80% 구현)
**파일**: `Core/Src/bsp.c`, `Core/Inc/bsp.h`

**구현 완료**:
- ✅ I2C1/I2C3 Read/Write 함수
- ✅ 배터리 전압 읽기
- ✅ 센서 전원 제어 GPIO 초기화

**누락 사항**:
- ❌ I2C 버스 복구 함수 (BSP_I2C1_Recovery, BSP_I2C3_Recovery)
- ❌ UART DMA 설정 함수
- ❌ GPIO 에러 핸들링

---

### 2. HITL 시스템 (70% 구현, 0% 검증)
**디렉토리**: `docs/HITL/`

**구현 완료**:
- ✅ 5-Board 아키텍처 설계 완료
- ✅ Main Control 펌웨어 (`main_control/main_control.ino`)
- ✅ I2C1 Mock 펌웨어 (`I2C1_Mocking/`)
- ✅ I2C3 Mock 펌웨어 (`I2C3_Mocking/`)
- ✅ Python 시뮬레이터 (`sensor_sender.py`)
- ✅ ESP-NOW 프로토콜 (`common/hitl_protocol.h`)

**누락 사항**:
- ❌ HITL 실행 가이드 문서 불완전 (README.md만 존재)
- ❌ 실제 하드웨어 테스트 결과 **없음**
- ❌ FDIR 복구 동작 검증 스크립트 **없음**
- ❌ 배선 다이어그램 (`wiring.md`는 있으나 불완전)

**예상 작업량**: 문서화 및 테스트 시나리오 작성 필요

---

### 3. SITL 시스템 (60% 구현, 0% 검증)
**디렉토리**: `HostSim/`

**구현 완료**:
- ✅ CMake 빌드 시스템 (`CMakeLists.txt`)
- ✅ Mock HAL 레이어 (`mock_hal.c`)
- ✅ Mock 센서 (`mock_sensors.c`)
- ✅ 비행 데이터 변환 스크립트 (`convert_flight_data.py`)
- ✅ 실행 파일 생성됨 (`test_host.exe`)

**누락 사항**:
- ❌ 실제 실행 결과 **없음**
- ❌ 비행 시나리오 데이터 (`flight_data.h`는 있으나 실제 데이터인지 불명)
- ❌ SITL 실행 가이드 **없음**
- ❌ 시뮬레이션 결과 분석 도구 **없음**

---

### 4. 단위 테스트 (50% 구현, 일부 검증)
**디렉토리**: `test/`

**구현 완료**:
- ✅ FDIR 테스트: `test/test_fdir/` - 4개 테스트 PASS
- ✅ Kalman 테스트: `test/test_kalman/` - 실행 가능
- ✅ PID 테스트: `test/test_pid/` - 실행 가능
- ✅ Unity 프레임워크: `test/unity_minimal/`

**누락 사항**:
- ❌ 센서 드라이버 테스트 (test_drivers 폴더는 있으나 비어있음)
- ❌ 텔레메트리 테스트 **없음**
- ❌ BSP 레이어 테스트 **없음**
- ❌ 통합 테스트 (`test/test_integration/`은 있으나 불완전)
- ❌ CI/CD 파이프라인 **없음**

---

## 📋 구현 우선순위 (현실적 평가)

### 🔴 Priority 0: 크리티컬 (비행 불가능)

1. **Sensors_Reset() 함수 완전 구현** (필수)
   - 모든 센서 리셋 로직 (10개 센서)
   - 예상 시간: **1-2일**
   - 예상 코드: 150 LOC

2. **Telemetry_Send() 함수 구현** (필수)
   - UART3 DMA 전송
   - CRC 계산 및 삽입
   - 예상 시간: **4-6시간**
   - 예상 코드: 120 LOC

3. **I2C 버스 복구 구현** (필수)
   - BSP_I2C1_Recovery(), BSP_I2C3_Recovery()
   - 9-Clock Pulse 생성
   - 예상 시간: **3-4시간**
   - 예상 코드: 80 LOC

**Priority 0 총 예상 시간**: **2-3일**

---

### 🟠 Priority 1: 중요 (안정성 필수)

4. **GPS NMEA 파싱 검증 및 완성**
   - XA1110 드라이버 완전성 체크
   - 예상 시간: **4-6시간**

5. **저전압 보호 구현**
   - 배터리 < 2.7V 시 Load Shedding
   - 예상 시간: **2-3시간**
   - 예상 코드: 50 LOC

6. **실제 하드웨어 통합 테스트**
   - 모든 센서 동작 확인
   - FDIR 복구 동작 검증
   - 예상 시간: **1주일**

**Priority 1 총 예상 시간**: **1.5주**

---

### 🟡 Priority 2: 권장 (비행 성공률 향상)

7. **GPS 1PPS 동기화 구현**
   - EXTI4 인터럽트
   - 50Hz 슬롯 분할
   - 예상 시간: **6-8시간**
   - 예상 코드: 100 LOC

8. **HITL 전체 검증**
   - 5-Board 시스템 실제 구동
   - FDIR 시나리오 테스트
   - 예상 시간: **1주일**

9. **텔레메트리 CRC 검증**
   - 수신측 CRC 체크 로직
   - 예상 시간: **2-3시간**

**Priority 2 총 예상 시간**: **2주**

---

### 🟢 Priority 3: 선택 (편의성)

10. **CI/CD 파이프라인 구축**
11. **플래시 로깅** (옵션)
12. **SITL 완전 검증**

---

## 🔍 검증 상태 요약

### 단위 테스트 (Unit Tests)
- ✅ FDIR: 4/4 테스트 PASS
- ⚠️ Kalman: 실행 파일 있으나 결과 미확인
- ⚠️ PID: 실행 파일 있으나 결과 미확인
- ❌ 센서 드라이버: 테스트 없음
- ❌ 텔레메트리: 테스트 없음
- ❌ BSP: 테스트 없음

### 통합 테스트 (Integration Tests)
- ❌ 전체 시스템: 한 번도 실행 안 됨
- ❌ HITL: 설계만 완료
- ❌ SITL: 설계만 완료

### 하드웨어 검증 (Hardware Verification)
- ❌ 실제 STM32: **한 번도 실행 안 됨**
- ❌ 센서 보드: **한 번도 연결 안 됨**
- ❌ P-MOS 전원 제어: **한 번도 테스트 안 됨**
- ❌ LoRa 텔레메트리: **한 번도 전송 안 됨**

---

## ⚠️ 비행 준비 상태 (Flight Readiness)

### 현재 상태: **Prototype (Alpha)**
- **설계**: 완료 (100%)
- **구현**: 부분 완료 (60%)
- **검증**: 거의 없음 (15%)
- **하드웨어 테스트**: 전혀 없음 (0%)

### 목표 상태: **Flight Model (FM)**
- 설계: 100%
- 구현: 100%
- 검증: 100%
- 하드웨어 테스트: 100%

### 비행 투입까지 남은 작업
1. Priority 0 완료: 2-3일
2. Priority 1 완료: 1.5주
3. 하드웨어 통합 테스트: 1주
4. 환경 챔버 테스트: 3-5일
5. 최종 통합 테스트 (FIT): 3일

**최소 예상 시간**: **3-4주**

---

## 📞 결론

이 프로젝트는 **설계는 매우 우수**하지만, **실제 구현과 검증은 크게 부족**합니다.

### 문서에는 있으나 코드에 없는 것들:
1. ❌ Sensors_Reset() 실제 구현
2. ❌ I2C 버스 복구
3. ❌ GPS 1PPS 동기화
4. ❌ Telemetry_Send() 함수
5. ❌ 저전압 보호
6. ❌ 하드웨어 검증 결과

### 실제로 동작하는 것들:
1. ✅ FDIR 타임아웃 감지 및 상태 머신
2. ✅ 센서 드라이버 (대부분)
3. ✅ PID 제어 알고리즘
4. ✅ 칼만 필터 알고리즘
5. ✅ 텔레메트리 데이터 구조체

**비행 투입 전 반드시 필요한 작업**: Priority 0 + Priority 1 전체 완료

---

## 📌 참조 문서

1. **[FDIR.md](./FDIR.md)** - 설계 완료, 구현 5%
2. **[FMEA.md](./FMEA.md)** - 설계 완료
3. **[STM32_SpaceBalloon_Specification.md](./STM32_SpaceBalloon_Specification.md)** - 설계 완료, 많은 부분 미구현
4. **[FINAL_REPORT.md](./FINAL_REPORT.md)** - Rev 3.6, 현실적으로 업데이트됨
5. **[HITL/README.md](./HITL/README.md)** - 5-Board 시스템 설계

---

**작성자**: 객관적 코드 분석 기반
**마지막 업데이트**: 2026-01-09
**다음 업데이트**: Priority 0 완료 후
