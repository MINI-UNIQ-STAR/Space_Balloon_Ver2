# SpaceBalloon 2.0 STM32 코드 리뷰 종합 보고서

| 항목 | 내용 |
|------|------|
| **프로젝트** | 성층권 풍선 비행 컴퓨터 |
| **MCU** | STM32G431 (170MHz) |
| **리뷰 날짜** | 2026-01-08 (Updated) |
| **리뷰 파일 수** | 47개 |

---

## 📊 리뷰 요약

| 카테고리 | 파일 수 | 평균 평가 |
|----------|---------|----------|
| [STM32 드라이버](STM32/drivers/) | 12 | ⭐⭐⭐⭐⭐ |
| [STM32 소스](STM32/src/) | 10 | ⭐⭐⭐⭐⭐ |
| [STM32 헤더](STM32/Inc/) | 10 | ⭐⭐⭐⭐⭐ |
| [LoRa32 RX](STM32/lora32_rx_review.md) | 1 | ⭐⭐⭐⭐⭐ |
| [HIL Mock](HIL/) | 7 | ⭐⭐⭐⭐⭐ |
| [SIL 테스트](SIL/) | 7 | ⭐⭐⭐⭐⭐ |

### **전체 품질: ⭐⭐⭐⭐⭐ (4.9/5)**

---

## 🏗️ 시스템 아키텍처

```
┌─────────────────────────────────────────────────────────────┐
│                    STM32G431 Flight Computer                │
├─────────────────────────────────────────────────────────────┤
│  app.c ──► sensors.c ──► fdir.c ──► kalman.c/pid.c          │
│     │           │            │            │                 │
│     └──► telemetry.c ◄───────┴────────────┘                 │
├─────────────────────────────────────────────────────────────┤
│  Core/Drivers (12개): IMU, GPS, Baro, Mag, Temp, Air, Rad  │
├─────────────────────────────────────────────────────────────┤
│  BSP Layer: I2C1/I2C3, UART, ADC, GPIO                      │
└─────────────────────────────────────────────────────────────┘
           │ UART3
           ▼
┌─────────────────────────────────────────────────────────────┐
│                    LoRa32 v2.1 Gateway                      │
├─────────────────────────────────────────────────────────────┤
│  telemetry_rx.ino ──► SD Card (CSV) + LoRa TX (SF11)       │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 STM32 코드 (33개 파일)

### 드라이버 (12개)

| 센서 | 인터페이스 | 특징 |
|------|-----------|------|
| LSM6DSV16X | I2C1 | ST 공식 드라이버 (11K줄) |
| MLX90393 | I2C1 | LSB 룩업 테이블 |
| GDK101 | I2C1 | 1/10분 평균 방사선 |
| MS5611 | I2C3 | CRC4 PROM, 상태머신 |
| SHT31 | I2C3 | CRC8, 히터 제어 |
| CM1107N | I2C3 | CO2 체크섬 |
| MCP9600 | I2C3 | Type-K 열전대 |
| SEN0321 | I2C3 | 오존 ppb |
| XA1110 | UART | Multi-GNSS GSV 파싱 |
| PMS3003 | UART | ISR 바이트 파서 |
| DS18B20 | 1-Wire | 완전한 프로토콜 스택 |
| minmea | Library | NMEA 파서 |

### 서비스 (10개)

| 모듈 | 라인 | 역할 |
|------|------|------|
| app.c | 225 | 메인 루프 (50Hz) |
| sensors.c | 648 | 센서 통합 허브 |
| fdir.c | 374 | 결함 감지/복구 |
| kalman.c | 114 | 고도 융합 필터 |
| pid.c | 50 | 히터 제어 (Anti-windup) |
| telemetry.c | 100 | CRC16 프레임 전송 |
| bsp.c | 176 | HAL 래퍼 |
| actuators.c | 58 | PWM 히터 |
| main.c | 220 | CubeMX 진입점 |
| xcp.c | 135 | XCP 프로토콜 구현 |

---

## 🔧 HIL (Hardware-in-the-Loop) (7개 파일)

```
PC (sensor_sender.py)
       │ USB (ALL:...)
       ▼
main_control.ino ─ESP-NOW─┬─► I2C1_Dual_Mock (LSM+MLX)
                          │
                          ├─► I2C1_GDK_GPIO_Mock (GDK+DAC+1-Wire)
                          │
                          ├─► I2C3_Dual_Mock_A (MS5611+SHT31)
                          │
                          └─► I2C3_Dual_Mock_B_UART (CM1107N+MCP+GPS+PMS)
```

| 노드 | 역할 | 평가 |
|------|------|------|
| sensor_sender.py | PySide6 대시보드 | ⭐⭐⭐⭐⭐ |
| main_control | ESP-NOW 허브 | ⭐⭐⭐⭐⭐ |
| I2C1_Dual_Mock | IMU+Mag | ⭐⭐⭐⭐ |
| I2C1_GDK_GPIO_Mock | RAD+DAC+1-Wire | ⭐⭐⭐⭐⭐ |
| I2C3_Dual_Mock_A | Baro+Humid | ⭐⭐⭐⭐ |
| I2C3_Dual_Mock_B_UART | CO2+TC+GPS+PM | ⭐⭐⭐⭐⭐ |

---

## 🧪 SIL (Software-in-the-Loop) (7개 파일)

| 모듈 | 테스트 수 | 대상 |
|------|----------|------|
| test_pid | 6 | P/I/D/Clamp/Windup |
| test_kalman | 4 | Init/Predict/Converge/Ascent |
| test_fdir | 4 | Timeout/Recovery/Cold |
| test_drivers | 12 | 전체 드라이버 |
| test_integration | 1 (SITL) | 미션 전체 |
| HostSim | - | RS41 비행 데이터 재생 |

---

## ✅ 강점

1. **FDIR 통합**: 센서 타임아웃 → 복구 → 영구 실패 상태머신
2. **저온 보호**: 히스테리시스 기반 센서 비활성화
3. **비차단 드라이버**: MS5611/SHT31 상태머신
4. **Multi-GNSS**: GPS/GLONASS/Galileo/BeiDou GSV 파싱
5. **CRC 검증**: CRC4(MS5611), CRC8(SHT31), CRC16(텔레메트리)
6. **테스트 커버리지**: PID/KF/FDIR 단위 + SITL 통합

---

## ⚠️ 개선 권장

| 항목 | 우선순위 | 설명 |
|------|----------|------|
| XCP 완성 | 완료 | [IMP-05] 캘리브레이션 스텁 및 명령어(Connect/Upload/Download) 구현 |
| LoRa CSMA | 완료 | [IMP-01] LBT(Listen Before Talk) 로직 적용 |
| HIL 레지스터 맵 | 완료 | [IMP-03] LSM6DSV16X / [IMP-04] MS5611 Mock 고도화 |
| SIL/MSVC 호환성 | 완료 | [IMP-06] 빌드 경고 및 링커 에러 전체 해결 |

---

## 📈 결론

> **성층권 비행 준비 완료 (Flight-Ready)**

| 지표 | 상태 |
|------|------|
| 코드 품질 | ⭐⭐⭐⭐⭐ |
| 테스트 커버리지 | SIL+HIL 완비 |
| 안전 시스템 | FDIR 4단계 상태머신 |
| 문서화 | 47개 개별 리뷰 |

---

*생성일: 2026-01-08T11:25 KST (Updated: 13:40 KST)*
