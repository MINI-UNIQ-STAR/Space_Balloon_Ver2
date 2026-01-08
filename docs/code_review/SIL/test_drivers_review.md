# test_drivers 단위 테스트 리뷰

| 항목 | 내용 |
|------|------|
| **프레임워크** | Unity |
| **대상** | 12개 센서 드라이버 |
| **파일** | test_main.c (158줄) |

---

## 테스트 케이스

| 테스트 | 대상 |
|--------|------|
| LSM6DSV16X | WHO_AM_I, Init |
| MLX90393 | Transceive, Measurement |
| XA1110 | NMEA Parsing, RMC/GGA/GSV |
| GDK101 | Radiation Read |
| MS5611 | State Machine, CRC4 |
| SHT31 | Humidity, CRC8 |
| PMS3003 | Stream Parsing |

---

## Mock HAL

```c
// test_drivers/mock_hal.c
HAL_StatusTypeDef HAL_I2C_Mem_Read(...) {
    // 센서별 Mock 데이터 반환
}
```

---

## 평가: ⭐⭐⭐⭐ (4/5)

**12개 드라이버 포괄. MSVC 호환성 이슈 (ST 외부 드라이버) 존재.**
