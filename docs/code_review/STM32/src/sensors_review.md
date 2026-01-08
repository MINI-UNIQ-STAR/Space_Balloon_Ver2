# sensors.c 센서 관리 코드 리뷰

| 항목 | 내용 |
|------|------|
| **레이어** | Service |
| **코드 라인** | 635줄 (가장 큰 파일) |
| **역할** | 센서 드라이버 통합 관리 |

## 구조

| 그룹 | 함수 | 설명 |
|------|------|------|
| **Init** | `Sensors_Init()`, `_I2C1/3()`, `_UART()`, `_1Wire()` | 초기화 |
| **Read** | `Read_IMU/Mag/Baro/Humid/GPS/...` | 개별 읽기 |
| **Control** | `SetHeater_SHT31()`, `Reset()` | 제어 |
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
- ✅ 각 `Read_*()` 함수에서 `FDIR_ReportSuccess()` 호출
- ✅ 드라이버 반환값 검사 (LSM/MLX)

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **추상화** | ⭐⭐⭐⭐⭐ | 드라이버 통합 |
| **데시메이션** | ⭐⭐⭐⭐⭐ | 비차단 |
| **Mock** | ⭐⭐⭐⭐⭐ | HOST_TEST_MODE |
| **FDIR** | ⭐⭐⭐⭐⭐ | 연동 |

## 종합: ⭐⭐⭐⭐⭐ (5/5)

**센서 시스템 중앙 허브.**
