# I2C1_GDK_GPIO_Mock.ino 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | ESP32 DevKit |
| **코드 라인** | 115줄 |
| **역할** | GDK101 + GPIO (1-Wire, DAC, Reset) Mock |

---

## 기능 구성

| 기능 | 핀 | 설명 |
|------|-----|------|
| **I2C (GDK101)** | 21/22 | 방사선 센서 (0x18) |
| **DAC (Battery)** | 25 | 6:1 분압 역산 |
| **1-Wire** | 4 | DS18B20 Mock (BitBang) |
| **Heaters** | 18/19 | PWM 입력 감지 |
| **Resets** | 23,26,27... | 센서 리셋 GPIO |

---

## 1-Wire Mock 로직

```c
void runOneWireMock() {
  if (digitalRead(ONE_WIRE_PIN) == LOW) {
    // Reset Pulse 감지 → Presence Pulse 응답
    delayMicroseconds(30);
    digitalWrite(ONE_WIRE_PIN, LOW);  // Presence
    delayMicroseconds(120);
  }
}
```

---

## DAC Mock

```c
uint16_t dac_mv = mock_bat / 6;  // 6:1 분압 역산
dacWrite(BAT_DAC_PIN, map(dac_mv, 0, 3300, 0, 255));
```

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**I2C + GPIO + DAC + 1-Wire 통합 Mock.**
