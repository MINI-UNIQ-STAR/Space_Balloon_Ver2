# HostSim 코드 리뷰

| 항목 | 내용 |
|------|------|
| **역할** | Host PC 시뮬레이션 환경 |
| **파일 수** | 5개 핵심 파일 |

---

## 파일 구조

| 파일 | 라인 | 역할 |
|------|------|------|
| `mock_hal.c` | 55줄 | HAL 스텁 (I2C/GPIO/UART/ADC) |
| `mock_sensors.c` | 304줄 | 센서 시뮬레이션 (RS41 비행 데이터) |
| `flight_data.h` | ~6KB | 실제 라디오존데 비행 궤적 |
| `convert_flight_data.py` | ~150줄 | JSON→C 변환 |

---

## mock_hal.c 핵심

```c
uint32_t HAL_GetTick(void) { return mock_tick; }
void HAL_Delay(uint32_t ms) { mock_tick += ms; }
void MockHAL_AdvanceTick(uint32_t ms) { mock_tick += ms; }
```
- **시간 제어**: `MockHAL_AdvanceTick()`으로 테스트 시간 조작
- **I2C/UART**: 빈 스텁 (OK 반환)

---

## mock_sensors.c 핵심

```c
// RS41 비행 데이터 재생
const flight_data_point_t *cur = &flight_data[current_frame];
data->gps_alt_m = lerp(cur->alt_m, next->alt_m, t);
data->ms5611_press_pa = ISA_formula(alt);
```
- **비행 데이터 재생**: 50Hz 보간
- **결함 주입**: `MockSensors_InjectFault()`
- **ISA 기압 계산**: `101325 * (1 - alt/44330)^5.255`

---

## 평가: ⭐⭐⭐⭐⭐ (5/5)

**완전한 Host 시뮬레이션 환경. RS41 실제 비행 데이터 기반.**
