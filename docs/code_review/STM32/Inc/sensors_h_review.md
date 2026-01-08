# sensors.h 센서 관리 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 77줄 |
| **역할** | 센서 추상화 인터페이스 |

## 센서 ID 열거형
```c
typedef enum {
    SENSOR_ID_IMU, SENSOR_ID_MAG, SENSOR_ID_BARO,
    SENSOR_ID_GPS, SENSOR_ID_PMS, SENSOR_ID_CO2,
    SENSOR_ID_SHT, SENSOR_ID_RAD, SENSOR_ID_EXT_TEMP,
    SENSOR_ID_COUNT
} SensorID_t;
```

## API 그룹

| 그룹 | 함수 |
|------|------|
| **Init** | `Sensors_Init`, `_I2C1/3`, `_UART`, `_1Wire` |
| **Read** | `_IMU`, `_Mag`, `_Baro`, `_GPS` 등 |
| **Control** | `SetHeater_SHT31`, `Reset` |

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **ID 열거** | ⭐⭐⭐⭐⭐ | FDIR 연동 |
| **GPS API** | ⭐⭐⭐⭐⭐ | Multi-GNSS GSA/GSV 지원 |

## 최근 개선사항
- ✅ **Multi-GNSS 지원 확장**: GPS/GLONASS/Galileo/BeiDou 위성 수 추적 (IMP-09)
- ✅ **위성 수 텔레메트리 추가**: `gps_sats_total`/`gps_sats_used` 필드 추가

## 종합: ⭐⭐⭐⭐⭐ (5/5)
