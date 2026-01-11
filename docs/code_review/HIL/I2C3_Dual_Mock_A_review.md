# I2C3_Dual_Mock_A.ino 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | LoRa32 v2.1 |
| **코드 라인** | 87줄 |
| **역할** | MS5611 + SHT31 I2C Mock |

---

## 핀 구성

| 포트 | 센서 | SDA | SCL | 주소 |
|------|------|-----|-----|------|
| Wire0 | MS5611 | 21 | 22 | 0x77 |
| Wire1 | SHT31 | 13 | 12 | 0x44 |

---

## I2C 응답

### MS5611
```c
if (ms_byte == 0x00) {
  // ADC Read: 3 bytes
  Wire.write(0x80); Wire.write(0x00); Wire.write(0x00);
} else if (ms_byte >= 0xA0) {
  // PROM Read: 2 bytes
  // PROM Read: 2 bytes (C1-C6 implemented)
  Wire.write(prom[idx] >> 8); Wire.write(prom[idx] & 0xFF);
}
```
**ADC 역산 로직 추가 (IMP-04)**: `mock_temp`/`mock_press` → `D1`/`D2` 변환 제공.

### SHT31
```c
// 6 bytes: Temp(2) + CRC + Hum(2) + CRC
uint8_t buf[6] = {0x66, 0x66, 0x00, 0x88, 0x88, 0x00};
Wire1.write(buf, 6);
```

---

## 평가: ⭐⭐⭐⭐ (4/5)

**듀얼 I2C Mock. MS5611 역산 로직 및 PROM 계수 구현 완료 (IMP-04).**
