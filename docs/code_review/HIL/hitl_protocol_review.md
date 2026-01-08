# hitl_protocol.h 공통 프로토콜 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **위치** | common/, main_control/ |
| **역할** | ESP-NOW 패킷 구조체 정의 |

---

## 구조체

```c
typedef struct {
  uint32_t timestamp_ms;
  float accel[3], gyro[3], mag[3];
  float temp_c, ext_temp_c, humidity;
  uint32_t pressure_pa;
  int32_t lat_e7, lon_e7;
  float alt_m;
  uint16_t co2, pm2_5, bat_mv;
  int16_t ozone_ppb;
  float radiation;
  uint8_t fault_comp, fault_type;
} HitlStatePacket;

typedef struct {
  uint8_t node_id;
  uint8_t heater_bat_duty, heater_bd_duty;
  uint8_t reset_flags;
} HitlFeedbackPacket;
```

---

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **구조** | ⭐⭐⭐⭐⭐ | 전체 센서 커버 |
| **패킹** | ⭐⭐⭐⭐ | __attribute__((packed)) 없음 |
| **결함 필드** | ⭐⭐⭐⭐⭐ | fault_comp/type |

---

## 종합: ⭐⭐⭐⭐⭐ (5/5)
