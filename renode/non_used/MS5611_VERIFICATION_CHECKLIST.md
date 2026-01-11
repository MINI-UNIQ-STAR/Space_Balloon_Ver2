# MS5611 기압계 센서 검증 체크리스트

**프로젝트**: STM32G431 Space Balloon Radiosonde
**작업**: Phase 5 - MS5611 센서 DLL 구현
**날짜**: 2026-01-11
**에이전트**: renode-simulation-engineer

---

## 개요

이 체크리스트는 MS5611 기압계 센서 DLL 구현을 검증하기 위한 것입니다.
**성층권 압력/온도 시뮬레이션이 International Standard Atmosphere 모델과 일치하는지 반드시 확인해야 합니다.**

---

## 1. MS5611 데이터시트 준수 검증

### 1.1 I2C 프로토콜

**데이터시트**: TE Connectivity MS5611-01BA03

- [ ] **I2C 주소**: 0x77 (CSB = HIGH)
- [ ] **Reset 명령**: 0x1E
- [ ] **PROM Read**: 0xA0, 0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC, 0xAE (16-bit 값)
- [ ] **Convert D1 (압력)**: 0x40-0x48 (OSR별)
- [ ] **Convert D2 (온도)**: 0x50-0x58 (OSR별)
- [ ] **ADC Read**: 0x00 (24-bit 값, 3 바이트)

### 1.2 PROM 캘리브레이션 계수

**파일**: `renode/sensors/ms5611_i2c.cs` (147-154행)

**현재 값**:
```csharp
prom[0] = 0x3132;  // Factory reserved
prom[1] = 0xA2E0;  // C1 = 41696 (SENS_T1)
prom[2] = 0x9188;  // C2 = 37256 (OFF_T1)
prom[3] = 0x5B93;  // C3 = 23443 (TCS)
prom[4] = 0x5D1D;  // C4 = 23837 (TCO)
prom[5] = 0x7D8F;  // C5 = 32143 (T_REF)
prom[6] = 0x6D0F;  // C6 = 27919 (TEMPSENS)
prom[7] = (CRC << 12);  // CRC-4
```

**검증**:
- [ ] C1-C6 값이 MS5611 일반적 범위 내 (데이터시트 확인)
- [ ] CRC-4 계산 알고리즘이 정확 (157-175행)
- [ ] Big-endian 바이트 순서 (100-105행)

### 1.3 압력/온도 계산 알고리즘

**MS5611 정방향 알고리즘** (데이터시트 기준):
```
dT = D2 - C5 * 2^8
TEMP = 2000 + dT * C6 / 2^23
OFF = C2 * 2^16 + (C4 * dT) / 2^7
SENS = C1 * 2^15 + (C3 * dT) / 2^8
P = (D1 * SENS / 2^21 - OFF) / 2^15
```

**검증** (역변환 구현, 235-269행):
- [ ] D2 계산이 정확: `D2 = dT + C5 * 256`
- [ ] D1 계산이 정확: `D1 = ((P * 32768 + OFF) * 2097152) / SENS`
- [ ] 24-bit 범위 제한 (0x000000 ~ 0xFFFFFF)
- [ ] 단위 변환 정확:
  - 압력: mbar → 0.01 mbar 단위
  - 온도: °C → 0.01°C 단위

---

## 2. 성층권 시뮬레이션 검증 (중요!)

### 2.1 International Standard Atmosphere (ISA) 모델

**파일**: `renode/sensors/ms5611_i2c.cs` (195-232행)

#### 대류권 (0-11km)

**현재 구현**:
```csharp
temperature_K = 288.15 - 0.0065 * h
pressure_Pa = 101325.0 * Math.Pow(temperature_K / 288.15, 5.2561)
```

**검증**:
- [ ] 해수면 (0m): T=288.15K (15°C), P=101325 Pa (1013.25 mbar)
- [ ] 5km: T=255.65K (-17.5°C), P≈54048 Pa (540.5 mbar)
- [ ] 11km: T=216.65K (-56.5°C), P≈22632 Pa (226.3 mbar)
- [ ] 온도 감률: -6.5°C/km (정확)
- [ ] 압력 지수: 5.2561 (정확)

#### 성층권 하부 (11-20km, 등온층)

**현재 구현**:
```csharp
temperature_K = 216.65
pressure_Pa = 22632.0 * Math.Exp(-0.00015769 * (h - 11000))
```

**검증**:
- [ ] 온도 일정: 216.65K (-56.5°C)
- [ ] 20km: P≈5474.9 Pa (54.7 mbar)
- [ ] 지수 계수: -0.00015769 (정확)

#### 성층권 중부 (20-32km)

**현재 구현**:
```csharp
temperature_K = 216.65 + 0.001 * (h - 20000)
pressure_Pa = 5474.9 * Math.Exp(...) * factor
```

**검증**:
- [ ] 온도 증가율: +1°C/km (정확)
- [ ] 30km: T≈226.65K (-46.5°C), P≈1197 Pa (12.0 mbar)

#### 성층권 상부 (32-40km)

**현재 구현**:
```csharp
temperature_K = 228.65 + 0.0028 * (h - 32000)
pressure_Pa = 868.02 * Math.Pow(temperature_K / 228.65, -34.1632 / 0.0028)
```

**검증**:
- [ ] 온도 증가율: +2.8°C/km (정확)
- [ ] 40km: T≈251.05K (-22.1°C), P≈287 Pa (2.9 mbar)

### 2.2 풍선 상승 시뮬레이션

**파일**: 189-193행

**현재 값**:
- 상승 속도: 5.0 m/s
- 업데이트 주기: 0.1초 (10Hz)

**검증**:
- [ ] 상승 속도가 현실적 (기상 풍선 3-7 m/s)
- [ ] 최대 고도 제한: 40,000m
- [ ] 고도 = ascentRate * time

---

## 3. 실제 하드웨어와 비교 (선택사항, 강력 권장)

### 3.1 해수면 조건 테스트

**예상 값** (고도 0m):
- 압력: 1013.25 ± 1.5 mbar
- 온도: 15 ± 2°C (실내)
- D1: ~9,000,000
- D2: ~8,500,000

- [ ] 실제 MS5611 센서로 측정
- [ ] 시뮬레이션 값과 비교 (오차 < 2%)

### 3.2 PROM 읽기 검증

**I2C 시퀀스**:
1. Reset: Write [0x1E]
2. PROM C1: Write [0xA2], Read 2 bytes
3. PROM C5: Write [0xAA], Read 2 bytes

- [ ] 실제 센서 PROM 값 읽기
- [ ] 값이 전형적 범위 내인지 확인:
  - C1: 40127 ~ 45000
  - C2: 36000 ~ 42000
  - C5: 30000 ~ 34000

### 3.3 압력/온도 변환 테스트

**I2C 시퀀스**:
1. Convert D1: Write [0x48] (OSR 4096)
2. Wait 9.04 ms
3. ADC Read: Write [0x00], Read 3 bytes
4. Convert D2: Write [0x58]
5. Wait 9.04 ms
6. ADC Read: Write [0x00], Read 3 bytes

- [ ] 실제 센서에서 D1, D2 읽기
- [ ] MS5611 알고리즘으로 압력/온도 계산
- [ ] 시뮬레이션과 비교

---

## 4. I2C 통신 검증

### 4.1 바이트 순서

- [ ] **PROM Read**: Big-endian (MSB first)
  - 예: C1=0xA2E0 → Read [0xA2, 0xE0]
- [ ] **ADC Read**: Big-endian (MSB first)
  - 예: D1=0x4D5E50 → Read [0x4D, 0x5E, 0x50]

### 4.2 명령어 응답

- [ ] Reset (0x1E): 응답 없음
- [ ] PROM Read: 2바이트 응답
- [ ] Convert D1/D2: 응답 없음 (변환 시작)
- [ ] ADC Read: 3바이트 응답

### 4.3 변환 시간

**실제 하드웨어**:
- OSR 256: 0.6 ms
- OSR 4096: 9.04 ms

**시뮬레이션**:
- [ ] 즉시 완료 (conversionReady = true)
- [ ] 실제 하드웨어에서는 대기 필요

---

## 5. Renode 통합 테스트

### 5.1 DLL 빌드

```powershell
cd renode/build_dll
.\build.ps1
```

- [ ] 빌드 성공
- [ ] `bin/Release/SensorPeripherals.dll` 생성
- [ ] MS5611 클래스 포함 확인

### 5.2 Renode 로드 테스트

```bash
cd renode
renode --disable-xwt --console test_ms5611.resc
```

- [ ] DLL 로드 오류 없음
- [ ] 플랫폼 정의 로드 성공
- [ ] I2C3에 MS5611 센서 등록됨 (주소 0x77)

### 5.3 시뮬레이션 실행

```
(monitor) i2c3
```

**예상 출력**:
```
Available peripherals:
  ms5611 (Sensors.MS5611 @ 0x77)
```

- [ ] MS5611 센서가 I2C3에 등록됨
- [ ] 주소가 0x77로 올바름

---

## 6. 데이터 정확성 검증

### 6.1 ADC 값 범위

**전형적 범위** (MS5611):
- D1 (압력): 3,000,000 ~ 13,000,000
- D2 (온도): 6,000,000 ~ 10,000,000

**시뮬레이션 검증**:
- [ ] 해수면 (0m): D1 ≈ 9,000,000, D2 ≈ 8,500,000
- [ ] 11km: D1 ≈ 4,000,000, D2 ≈ 7,400,000
- [ ] 40km: D1 ≈ 3,200,000, D2 ≈ 7,100,000
- [ ] 24-bit 범위 내 (0 ~ 16,777,215)

### 6.2 압력/온도 정확성

**고도별 체크포인트**:

| 고도 (km) | 압력 (mbar) | 온도 (°C) | D1 (예상) | D2 (예상) |
|-----------|-------------|-----------|-----------|-----------|
| 0         | 1013.25     | 15.0      | 9,000,000 | 8,500,000 |
| 5         | 540.5       | -17.5     | 6,800,000 | 8,200,000 |
| 11        | 226.3       | -56.5     | 4,000,000 | 7,400,000 |
| 20        | 54.7        | -56.5     | 3,400,000 | 7,400,000 |
| 30        | 12.0        | -46.5     | 3,200,000 | 7,500,000 |
| 40        | 2.9         | -22.1     | 3,100,000 | 7,700,000 |

- [ ] 각 고도에서 압력 값이 ISA 모델과 일치 (오차 < 5%)
- [ ] 각 고도에서 온도 값이 ISA 모델과 일치 (오차 < 2°C)
- [ ] D1, D2 값이 현실적 범위 내

### 6.3 노이즈 및 변동성

**현재 구현** (215-217행):
```csharp
double noise = (random.NextDouble() - 0.5) * 0.01;  // ±0.5%
pressure_mbar *= (1.0 + noise);
temperature_C += noise * 2.0;
```

- [ ] 노이즈 레벨이 현실적 (MS5611 정확도: ±1.5 mbar)
- [ ] 연속 읽기 시 값이 약간 변동 (센서 노이즈 시뮬레이션)

---

## 7. 성층권 장시간 시뮬레이션

### 7.1 11km 도달 테스트

**시뮬레이션 시간**: 11000m / 5m/s = 2200초 ≈ 37분

```
emulation RunFor "00:37:00"
```

- [ ] 고도 11km 도달
- [ ] 압력 ≈ 226 mbar
- [ ] 온도 ≈ -56.5°C
- [ ] 안정적으로 시뮬레이션 계속됨

### 7.2 40km 도달 테스트

**시뮬레이션 시간**: 40000m / 5m/s = 8000초 ≈ 133분

```
emulation RunFor "02:13:00"
```

- [ ] 고도 40km 도달 및 제한 확인
- [ ] 압력 ≈ 2.9 mbar
- [ ] 온도 ≈ -22°C
- [ ] 메모리 누수 없음

---

## 8. 최종 승인

### 8.1 데이터시트 준수

- [ ] I2C 프로토콜 100% 정확
- [ ] PROM 값 및 CRC 정확
- [ ] 압력/온도 알고리즘 정확

### 8.2 ISA 모델 준수

- [ ] 대류권 공식 정확
- [ ] 성층권 공식 정확
- [ ] 고도별 체크포인트 모두 통과

### 8.3 개선 사항 기록

**즉시 수정 필요**:
```
[여기에 반드시 수정해야 할 사항 기록]
```

**향후 개선 사항**:
```
[여기에 나중에 개선할 사항 기록]
예: 2차 온도 보상 구현, 실제 센서 데이터 비교 테스트
```

### 8.4 승인 서명

- [ ] MS5611 데이터시트와 비교 검증됨
- [ ] ISA 모델과 비교 검증됨
- [ ] 성층권 시뮬레이션이 정확함
- [ ] 모든 중요 항목이 체크됨

**검증자**: ____________________
**날짜**: ____________________
**서명**: ____________________

---

## 9. 참고 자료

### 데이터시트

- **MS5611-01BA03**: https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FMS5611-01BA03%7FB3%7Fpdf

### 대기 모델

- **International Standard Atmosphere**: https://en.wikipedia.org/wiki/International_Standard_Atmosphere
- **NASA Standard Atmosphere**: https://ntrs.nasa.gov/citations/19770009539

### 기상 풍선 데이터

- **NOAA Radiosonde**: https://www.weather.gov/upperair/factsheet
- **High Altitude Balloon**: https://ukhas.org.uk/

---

**중요 알림**: 성층권 압력/온도 시뮬레이션은 반드시 ISA 모델 및 실제 기상 데이터와 비교해서 검증해 주세요.
