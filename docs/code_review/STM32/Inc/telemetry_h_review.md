# telemetry.h 텔레메트리 헤더 리뷰

| 항목 | 내용 |
|------|------|
| **라인 수** | 104줄 |
| **역할** | 텔레메트리 프레임 정의 |

## 페이로드 구조체 (116 바이트)

| 섹션 | 필드 |
|------|------|
| **시스템** | uptime_ms, status_flags, co2_ppm |
| **IMU** | accel_mps2_x1000[3], gyro_rads_x1000[3] |
| **자기계** | mag_uT[3] |
| **온도** | board/external/sht31/bat _temp_c_x100 |
| **GPS** | lat/lon_e7, alt_m, fix, sats_used/total |
| **GPS 위성별** | sats_gps/glonass/galileo/beidou |
| **GPS UTC** | utc_hour/min/sec/day/month/year |
| **공기질** | pm1/25/10, ozone_ppb |
| **기압** | ms5611_press_pa, rh_x100 |
| **방사선** | gdk101_usvh_x100 |
| **고도** | press_alt_m, kf_alt_m, roll/pitch |

## 프레임 헤더
```c
magic[2] = {0xA5, 0x5A}
version, msg_type, payload_len, seq
timestamp_ms, ...payload, crc16
```

## 평가

| 항목 | 점수 | 비고 |
|------|------|------|
| **#pragma pack** | ⭐⭐⭐⭐⭐ | 정렬 보장 |
| **x100/x1000** | ⭐⭐⭐⭐⭐ | 정수 전송 |

## 최근 개선사항
- ✅ **GPS 위성별 수 필드 추가**: GPS/GLONASS/Galileo/BeiDou 각각 분리
- ✅ **GPS UTC 시간 필드 추가**: hour/min/sec/day/month/year
- ✅ **페이로드 116바이트 확정**: LoRa32 RX와 동기화

## 종합: ⭐⭐⭐⭐⭐ (5/5)
