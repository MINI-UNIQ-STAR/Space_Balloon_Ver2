# I2C3_Dual_Mock_B_UART.ino 코드 리뷰

| 항목 | 내용 |
|------|------|
| **플랫폼** | LoRa32 v2.1 |
| **코드 라인** | 131줄 |
| **역할** | CM1107N + MCP9600 + GPS/PMS UART Mock |

---

## 핀 구성

| 인터페이스 | 센서 | 핀 |
|------------|------|-----|
| Wire0 | CM1107N (CO2) | 21/22, 0x31 |
| Wire1 | MCP9600 (TC) | 32/33, 0x60 |
| Serial1 | XA1110 (GPS) | TX:17, RX:16 |
| Serial2 | PMS3003 (PM) | TX:4, RX:15 |

---

## I2C 응답

### CM1107N (CO2)
```c
uint8_t resp[8] = {0x16, 0x05, 0x01, CO2_H, CO2_L, 0, 0, 0};
Wire.write(resp, 8);
```

### MCP9600 (Thermocouple)
```c
int16_t t_raw = (int16_t)(mcp_temp * 16.0);  // 0.0625 LSB
Wire1.write((t_raw >> 8) & 0xFF);
Wire1.write(t_raw & 0xFF);
```

---

## UART Mock

### GPS (1초 주기)
```c
GPS_SERIAL.println("$GPGGA,120000.00,3500.0000,N,12700.0000,E,1,10,1.0,100.0,M,...");
```

### PMS3003 (1초 주기)
```c
uint8_t buf[32] = {0x42, 0x4D, 0, 28, ...};  // 표준 32바이트 프레임
PMS_SERIAL.write(buf, 32);
```

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**I2C+UART 통합 Mock. 가장 복잡한 노드.**
