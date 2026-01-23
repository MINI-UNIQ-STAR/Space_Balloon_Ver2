# sensors.c 센서 관리 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Service |
| **코드 라인** | 918줄 |
| **역할** | 센서 드라이버 통합 관리 |
| **최종 업데이트** | 2026-01-18 |

## 구조

| 그룹 | 함수 | 설명 |
|------|------|------|
| **Init** | `Sensors_Init()`, `_I2C1/3()`, `_UART()`, `_1Wire()` | 초기화 |
| **Read** | `Read_IMU/Mag/Baro/Humid/GPS/...` | 개별 읽기 |
| **Control** | `SetHeater_SHT31()`, `Sensors_Reset()` | 제어 |
| **All** | `Sensors_Read_All()` | 일괄 읽기 |

## 센서 매핑

| 버스 | 센서 |
|------|------|
| **I2C1** | LSM6DSV16X, MLX90393, GDK101 |
| **I2C3** | SHT31, MS5611, CM1107N, MCP9600, SEN0321 |
| **UART** | XA1110, PMS3003 |
| **1-Wire** | DS18B20 |

## 데시메이션
```
IMU/Mag: 50Hz
Baro: 5Hz (200ms)
Env/Battery: 1Hz (1000ms)
```

## FDIR 통합
## FDIR 통합
- ✅ 각 `Read_*()` 함수에서 `FDIR_ReportSuccess()` 호출
- ✅ 드라이버 반환값 검사 (LSM/MLX/MS5611 등)
- ✅ `Sensors_ProcessReset()` 비차단 상태 머신 구현 (기존 차단형 HAL_Delay 제거)
- ✅ 22종 시뮬레이션 테스트를 통한 로직 검증 완료
- ✅ 11개 센서 통합 (I2C1, I2C3, UART, 1-Wire)
- ✅ 비차단 상태머신 기반 기압/온습도 읽기
- ✅ LSM6DSV16X SFLP (Sensor Fusion Low Power) 지원
- ✅ SHT31 히터 제어 (결로 방지, 전력 예산 0.1W 소모)

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **추상화** | ⭐⭐⭐⭐⭐ | 드라이버 통합 |
| **데시메이션** | ⭐⭐⭐⭐⭐ | 비차단 |
| **Mock** | ⭐⭐⭐⭐⭐ | HOST_TEST_MODE |
| **FDIR** | ⭐⭐⭐⭐⭐ | 연동 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**11종의 센서를 통합 관리하는 중앙 허브. 비차단 상태머신과 데시메이션을 통해 CPU 부하를 최적화하고 FDIR과의 긴밀한 연동을 통해 시스템 신뢰성을 확보함.**
