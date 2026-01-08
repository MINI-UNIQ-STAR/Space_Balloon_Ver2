# FDIR (Fault Detection, Isolation, and Recovery)

고고도 기구 라디오존데 시스템의 결함 감지, 격리 및 복구 설계 문서

---

## 📋 목차

- [개요](#개요)
- [FDIR 아키텍처](#fdir-아키텍처)
- [상태 머신](#상태-머신)
- [감지 메커니즘](#감지-메커니즘)
- [복구 전략](#복구-전략)
- [센서별 FDIR](#센서별-fdir)
- [시스템 레벨 FDIR](#시스템-레벨-fdir)
- [구현 세부사항](#구현-세부사항)

---

## 개요

FDIR(Fault Detection, Isolation, and Recovery)은 시스템이 결함 상황에서도 미션을 계속 수행할 수 있도록 하는 핵심 메커니즘입니다.

### 설계 목표

| 목표 | 설명 |
|------|------|
| **가용성** | 단일 센서 실패 시에도 미션 계속 |
| **자동 복구** | 수동 개입 없이 결함 복구 시도 |
| **그레이스풀 디그레이드** | 복구 불가 시 기능 축소 운영 |
| **로깅** | 모든 결함 이벤트 텔레메트리 전송 |

---

## FDIR 아키텍처

```
┌─────────────────────────────────────────────────────────────┐
│                      FDIR Controller                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  Detection  │──│  Isolation  │──│      Recovery       │  │
│  │   Module    │  │   Module    │  │       Module        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│         ▲                                     │              │
│         │                                     ▼              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              Sensor Health Registry                      ││
│  │  [IMU] [Baro] [GPS] [Temp] [Humid] [CO2] [PM] [Rad]     ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
         ▲                                     │
         │ Report Success/Failure              │ Reset Command
         │                                     ▼
┌─────────────────┐                   ┌─────────────────┐
│  Sensor Drivers │                   │   HAL / GPIO    │
└─────────────────┘                   └─────────────────┘
```

---

## 상태 머신

각 센서는 독립적인 FDIR 상태 머신을 가집니다.

```
                    ┌──────────────┐
     Success        │   HEALTHY    │◄─────────────────┐
   ┌───────────────►│   (정상)     │                  │
   │                └──────┬───────┘                  │
   │                       │ Timeout (3초)            │
   │                       ▼                          │
   │                ┌──────────────┐                  │
   │                │   WARNING    │                  │
   │                │   (경고)     │                  │
   │                └──────┬───────┘                  │
   │                       │ Reset 명령               │ Success
   │                       ▼                          │
   │                ┌──────────────┐                  │
   └────────────────│   RECOVERY   │──────────────────┘
                    │   (복구중)   │
                    └──────┬───────┘
                           │ 5회 실패
                           ▼
                    ┌──────────────┐
                    │  PERMANENT   │
                    │   FAILURE    │
                    │ (영구 실패)  │
                    └──────────────┘
```

### 상태 정의

| 상태 | 값 | 설명 |
|------|-----|------|
| `HEALTHY` | 0 | 정상 동작, 주기적 데이터 수신 |
| `WARNING` | 1 | 타임아웃 발생, 복구 대기 |
| `RECOVERY` | 2 | 복구 시도 중 |
| `FAILURE_PERMANENT` | 3 | 복구 불가, 기능 비활성화 |

---

## 감지 메커니즘

### 1. 타임아웃 기반 감지

```c
#define SENSOR_TIMEOUT_DEFAULT 3000  // 3초

if (now - last_valid_update_ms > SENSOR_TIMEOUT_DEFAULT) {
    state = FDIR_STATE_WARNING;
}
```

| 센서 | 예상 주기 | 타임아웃 | 비고 |
|------|----------|---------|------|
| **IMU** (LSM6DSV16X) | 2ms | 100ms | 중요, 빠른 감지 |
| **자기계** (MLX90393) | 20ms | 500ms | |
| **기압계** (MS5611) | 200ms | 1000ms | 고도 제어 핵심 |
| **GPS** (XA1110) | 1000ms | 5000ms | Cold Start 고려 |
| **미세먼지** (PMS3003) | 1000ms | 5000ms | 워밍업 필요 |
| **CO2** (CM1107N) | 1000ms | 5000ms | |
| **온습도** (SHT31-D) | 1000ms | 3000ms | |
| **방사선** (GDK101) | 1000ms | 3000ms | 10분 평균 사용 |
| **외부온도** (MCP9600) | 1000ms | 3000ms | 열전대 |

### 2. 값 범위 검증

```c
// 예: 기압계 범위 검증
if (pressure_pa < 1000 || pressure_pa > 110000) {
    FDIR_ReportFailure(SENSOR_ID_BARO, ERR_OUT_OF_RANGE);
}
```

| 센서 | 유효 범위 | 비고 |
|------|----------|------|
| 기압 | 1,000 ~ 110,000 Pa | 30km 고도까지 |
| 온도 | -80°C ~ +60°C | 성층권 조건 |
| GPS 고도 | -500 ~ 50,000 m | |
| 배터리 | 2,700 ~ 4,200 mV | 1S4P 18650 |

### 3. 연속성 검사

급격한 값 변화 감지 (센서 노이즈 또는 결함)

```c
// 예: 고도 점프 감지
if (abs(gps_alt - last_alt) > 500.0f) {  // 500m 이상 점프
    FDIR_ReportFailure(SENSOR_ID_GPS, ERR_VALUE_JUMP);
}
```

---

## 복구 전략

### 레벨별 복구 동작

| 레벨 | 동작 | 설명 |
|------|------|------|
| L1 | 재시도 | 동일 명령 재전송 |
| L2 | 소프트 리셋 | 드라이버 재초기화 |
| L3 | 하드 리셋 | GPIO 파워 사이클 |
| L4 | 영구 비활성화 | 해당 센서 무시 |

### 복구 시퀀스

**✅ 구현 완료 (2026-01-09) - Core/Src/sensors.c:238-348**

모든 센서에 대한 하드웨어 복구 로직이 완전히 구현되었습니다.

**실제 구현 예시 (Core/Src/sensors.c:238-348)**:
```c
void Sensors_Reset(SensorID_t id) {
#ifdef UNIT_TEST
    printf("FDIR: Resetting Sensor ID %d\n", id);
    return;
#endif

#ifndef UNIT_TEST
    switch(id) {
        case SENSOR_ID_GPS:
            // L3: 하드 리셋 (PA9: GPS_nRST - active low)
            HAL_GPIO_WritePin(XA1110_RST_GPIO_Port, XA1110_RST_Pin, GPIO_PIN_RESET);
            HAL_Delay(100);
            HAL_GPIO_WritePin(XA1110_RST_GPIO_Port, XA1110_RST_Pin, GPIO_PIN_SET);
            HAL_Delay(50);
            // L2: 소프트 리셋
            XA1110_Init(&xa_ctx);
            break;

        case SENSOR_ID_IMU:
            // L3: P-MOS 전원 사이클 (PB13: LSM_RST)
            HAL_GPIO_WritePin(LSM_RST_GPIO_Port, LSM_RST_Pin, GPIO_PIN_RESET);
            HAL_Delay(50);
            HAL_GPIO_WritePin(LSM_RST_GPIO_Port, LSM_RST_Pin, GPIO_PIN_SET);
            HAL_Delay(50);
            // L2: I2C 버스 복구
            BSP_I2C1_Recovery();
            break;

        case SENSOR_ID_BARO:
            // L3: P-MOS 전원 사이클 (PA5: MS_RST)
            HAL_GPIO_WritePin(MS_RST_GPIO_Port, MS_RST_Pin, GPIO_PIN_RESET);
            HAL_Delay(50);
            HAL_GPIO_WritePin(MS_RST_GPIO_Port, MS_RST_Pin, GPIO_PIN_SET);
            HAL_Delay(50);
            // L2: I2C 버스 복구
            BSP_I2C3_Recovery();
            break;

        // ... (전체 9개 센서에 대한 복구 로직 구현됨)

        default:
            // 기본 동작: I2C 버스 복구 시도
            if (id == SENSOR_ID_MAG || id == SENSOR_ID_IMU || id == SENSOR_ID_RAD) {
                BSP_I2C1_Recovery();
            } else {
                BSP_I2C3_Recovery();
            }
            break;
    }
#endif
}
```

**I2C 버스 복구 구현 (Core/Src/bsp.c:177-280)**:
```c
void BSP_I2C1_Recovery(void) {
#ifndef UNIT_TEST
    // I2C1: PA15 (SCL), PB9 (SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Step 1: Disable I2C peripheral
    HAL_I2C_DeInit(&hi2c1);

    // Step 2: Configure SCL as GPIO Output (Open-Drain)
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Step 3: Generate 9 clock pulses
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
        BSP_Delay_us(5);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
        BSP_Delay_us(5);
    }

    // Step 4: Re-configure SCL as I2C alternate function
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Step 5: Re-enable I2C peripheral
    HAL_I2C_Init(&hi2c1);
    HAL_Delay(10);
#endif
}
```

> **구현 현황 (2026-01-09)**:
> - ✅ `Sensors_Reset()` 함수: 9개 센서 모두 구현 완료 (110 LOC)
> - ✅ I2C 버스 복구: I2C1/I2C3 모두 구현 완료 (104 LOC)
> - ✅ 단위 테스트: FDIR 4/4 PASS
> - ⚠️ 하드웨어 검증: 실제 STM32 보드에서 미검증

---

## 센서별 FDIR

### IMU (LSM6DSV16X)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| I2C 통신 실패 | HAL_TIMEOUT | 드라이버 재초기화 |
| WHO_AM_I 불일치 | 레지스터 검증 | 하드 리셋 |
| 데이터 정체 | 타임스탬프 비교 | SW 리셋 명령 |

### GPS (XA1110)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| NMEA 파싱 실패 | CRC 오류 | 무시 (다음 문장 대기) |
| Fix 불가 | fix_type == 0 | 경고 유지 (복구 불필요) |
| 응답 없음 | 타임아웃 | 하드 리셋 (RST 핀) |

### 기압계 (MS5611)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| 보정 데이터 무효 | PROM CRC | 재초기화 |
| 범위 초과 | 값 검증 | 이전 값 사용 |
| I2C 타임아웃 | HAL_TIMEOUT | Soft/Hard Reset |

### 자기계 (MLX90393)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| I2C 응답 없음 | 타임아웃 (500ms) | 드라이버 재초기화 |
| 하드웨어 락업 | 연속 오류 | Hard Reset (PB14) |

### 미세먼지 (PMS3003)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| 데이터 미수신 | 타임아웃 (5s) | Hard Reset (SET 핀 토글) |
| 체크섬 오류 | 프레임 검증 | 해당 패킷 무시 |
| 응답 없음 (Cold) | 저온 (-10°C 미만) | 자동 비활성화 (Cold Disable) |

### 습도/온도 (SHT31-D)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| I2C 통신 실패 | 타임아웃 (3s) | Hard Reset (PB11 P-MOS) |
| 히터 오동작 | 온도/전류 분석 | 히터 강제 Off |

### 방사선 (GDK101)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| 값 미갱신 | 타임아웃 (3s) | I2C 재초기화, Hard Reset (PB2) |
| 노이즈 스파이크 | 이동 평균 필터 | 순간 값 무시 |

### 기타 센서 (CO2, MCP9600)

| 결함 모드 | 감지 방법 | 복구 동작 |
|----------|----------|----------|
| 통신 두절 | 타임아웃 (5s/3s) | 공통 I2C 버스 복구, 개별 Reset |
| 범위 이탈 | Range Check | 오류 카운트 증가 |

---

## 시스템 레벨 FDIR

### 미션 크리티컬 센서

| 우선순위 | 센서 | 대체 소스 |
|---------|------|----------|
| 1 | GPS | 없음 (필수) |
| 2 | 기압계 | GPS 고도 |
| 3 | IMU | 없음 (Kalman 정지) |
| 4 | 온도 센서 | 히터 비활성화 |

### 시스템 상태 플래그

텔레메트리의 `status_flags` 필드 (16비트):

| 비트 | 플래그 | 설명 |
|------|--------|------|
| 0 | `SYS_OK` | 전체 시스템 정상 |
| 1 | `GPS_WARN` | GPS 경고/실패 |
| 2 | `BARO_WARN` | 기압계 경고/실패 |
| 3 | `IMU_WARN` | IMU 경고/실패 |
| 4 | `TEMP_WARN` | 온도 센서 경고 |
| 5 | `HEATER_ACTIVE` | 히터 동작 중 |
| 6 | `LOW_BATTERY` | 배터리 저전압 |
| 7 | `FDIR_RECOVERY` | 복구 동작 진행 중 |
| 8 | `ALT_JUMP` | 고도 점프 감지 (연속성 오류) |
| 9 | `RANGE_ERROR` | 값 범위 초과 감지 |

---

## 구현 세부사항

### 헤더 파일 (`fdir.h`)

```c
typedef enum {
    FDIR_STATE_HEALTHY = 0,
    FDIR_STATE_WARNING,
    FDIR_STATE_RECOVERY,
    FDIR_STATE_FAILURE_PERMANENT
} FdirState_t;

typedef struct {
    uint32_t last_valid_update_ms;
    uint32_t error_count;
    uint32_t recovery_count;
    FdirState_t state;
    bool enabled;
} SensorHealth_t;
```

### 주요 API

| 함수 | 설명 |
|------|------|
| `FDIR_Init()` | FDIR 시스템 초기화 |
| `FDIR_Update()` | 주기적 상태 점검 (1Hz) |
| `FDIR_ReportSuccess(id)` | 유효 데이터 수신 보고 |
| `FDIR_ReportFailure(id, code)` | 오류 발생 보고 |

### 호출 흐름

```
main() → App_Init() → FDIR_Init()
                       ↓
main() → App_Loop() → Sensors_Read_All() → FDIR_ReportSuccess()
                       ↓
                    FDIR_Update() → 타임아웃 감지 → Sensors_Reset()
```

---

## FDIR 검증 (HITL)

FDIR 메커니즘은 HITL(Hardware-In-The-Loop) 시뮬레이션을 통해 검증됩니다.

### 검증 시나리오
1. **센서 타임아웃 주입**: Mock 보드에서 의도적으로 응답을 지연시켜 타임아웃 로직 트리거
2. **잘못된 값 주입**: 정상 범위를 벗어난 값(예: 고도 60km, 영하 100도)을 전송하여 값 검증 로직 확인
3. **I2C 버스 오류**: SCL/SDA 라인 강제 점유를 통해 버스 복구(9-clock pulse) 메커니즘 동작 확인
4. **전원 사이클**: 리셋 핀(GPIO) 동작 시 Mock 보드에서 리셋 신호 감지 및 재부팅 시퀀스 확인

---

## 참고 문서

- [FMEA.md](FMEA.md) - 고장 모드 영향 분석
- [HITL/README.md](HITL/README.md) - HITL 시뮬레이션 시스템
- [telemetry.h](../Core/Inc/telemetry.h) - 텔레메트리 프로토콜 (116 바이트)
