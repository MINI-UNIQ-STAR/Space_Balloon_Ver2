# I2C1_Dual_Mock.ino 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | LoRa32 v2.1 |
| **코드 라인** | 110줄 |
| **역할** | LSM6DSV16X + MLX90393 I2C Mock |

---

## 핀 구성

| 포트 | 센서 | SDA | SCL | 주소 |
|------|------|-----|-----|------|
| Wire0 | LSM6DSV16X | 21 | 22 | 0x6B |
| Wire1 | MLX90393 | 13 | 12 | 0x0C |

---

## ESP-NOW 수신

```c
void OnDataRecv(...) {
  acc[0] = pkt->accel[0]; acc[1] = pkt->accel[1]; acc[2] = pkt->accel[2];
  gyr[0] = pkt->gyro[0];  ...
  mag[0] = pkt->mag[0];   ...
}
```

---

## I2C Slave 응답

| 센서 | 레지스터 | 응답 |
|------|----------|------|
| LSM6DSV16X | 0x0F (WHO_AM_I) | 0x70 / R/W Registers (IMP-03) |
| MLX90393 | - | 0x00 (Status) |

---

## 평가: ⭐⭐⭐⭐ (4/5)

**듀얼 I2C Mock. LSM6DSV16X 레지스터 맵(256B) 확장 완료 (IMP-03).**
