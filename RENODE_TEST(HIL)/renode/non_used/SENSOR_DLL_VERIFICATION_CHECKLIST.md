# Renode 센서 DLL 사람 검증 체크리스트

**프로젝트**: STM32G431 Space Balloon Radiosonde
**작업**: Phase 4 - 센서 DLL 컴파일 시스템 구축
**날짜**: 2026-01-11
**에이전트**: renode-simulation-engineer

---

## 개요

이 체크리스트는 AI 에이전트가 구현한 센서 DLL 컴파일 시스템을 사람이 검증하기 위한 것입니다.
**특히 센서 모킹 데이터와 동작은 반드시 실제 데이터시트 및 하드웨어 동작과 비교하여 검증해야 합니다.**

---

## 1. 빌드 시스템 검증

### 1.1 파일 존재 확인

- [ ] `renode/build_dll/SensorPeripherals.csproj` 파일 존재
- [ ] `renode/build_dll/build.ps1` 파일 존재
- [ ] `renode/build_dll/README.md` 파일 존재
- [ ] `renode/stm32g431_with_sensors.repl` 파일 존재
- [ ] `renode/test_sensor_dll.resc` 파일 존재

### 1.2 빌드 테스트

**사전 준비**:
- [ ] .NET 6.0 SDK 이상 설치 확인 (`dotnet --version`)
- [ ] Renode 설치 확인 (`renode --version`)

**빌드 실행**:
```powershell
cd renode/build_dll
.\build.ps1
```

- [ ] 스크립트가 오류 없이 실행됨
- [ ] Renode 경로가 정확히 감지됨
- [ ] 필수 DLL 참조가 모두 발견됨
- [ ] `bin/Release/SensorPeripherals.dll` 파일이 생성됨
- [ ] DLL 크기가 0보다 큼 (정상 범위: 10-50KB)

**오류 발생 시**:
- [ ] Renode 경로를 수동으로 지정: `.\build.ps1 -RenodePath "정확한\경로\bin"`
- [ ] .csproj 파일의 `<RenodePath>` 항목을 시스템에 맞게 수정

---

## 2. Renode 통합 검증

### 2.1 기본 로딩 테스트

```bash
cd renode
renode --disable-xwt --console test_sensor_dll.resc
```

**확인 사항**:
- [ ] Renode가 정상적으로 시작됨
- [ ] DLL 로드 오류가 없음
- [ ] 플랫폼 정의가 정상 로드됨
- [ ] 펌웨어 ELF가 정상 로드됨
- [ ] 시뮬레이션이 2초 동안 실행됨
- [ ] I2C 로그가 출력됨

### 2.2 센서 인스턴스 확인

Renode 콘솔에서:
```
(monitor) i2c1
```

**예상 출력**:
```
Available peripherals:
  lsm6dsv16x (Sensors.LSM6DSV16X @ 0x6B)
```

- [ ] LSM6DSV16X 센서가 I2C1에 등록되어 있음
- [ ] 주소가 0x6B로 올바름

---

## 3. ⚠️ 센서 모킹 데이터 검증 (중요!)

### 3.1 LSM6DSV16X 센서 데이터 검증

**데이터시트 확인 필수**: [LSM6DSV16X Datasheet](https://www.st.com/resource/en/datasheet/lsm6dsv16x.pdf)

#### WHO_AM_I 레지스터

- **파일**: `renode/sensors/lsm6dsv16x_i2c.cs`
- **코드 위치**: 71-72행
- **현재 값**: 0x70
- [ ] 데이터시트와 값이 일치하는지 확인
- [ ] 레지스터 주소 0x0F가 올바른지 확인

#### 가속도계 데이터

**현재 구현** (98-104행):
```csharp
accelX = (short)(0.1 * Math.Sin(time * 2.0) * 32768 / 2.0);
accelY = (short)(0.1 * Math.Cos(time * 1.5) * 32768 / 2.0);
accelZ = (short)((1.0 + 0.05 * noise) * 32768 / 2.0);
```

**검증 항목**:
- [ ] **범위**: ±2g 설정이 맞는지 확인
- [ ] **스케일 팩터**: 32768 / 2.0 = 16384 LSB/g가 올바른지 확인
- [ ] **중력**: Z축 1g 시뮬레이션이 현실적인지 확인
- [ ] **단위**: raw 값이 16-bit signed integer 범위 내인지 확인 (-32768 ~ 32767)
- [ ] **레지스터 주소**: 0x28-0x2D가 올바른지 데이터시트 확인

**개선 필요 시**:
- [ ] 스케일 팩터 수정
- [ ] 시뮬레이션 값 범위 조정
- [ ] 노이즈 레벨 조정

#### 자이로스코프 데이터

**현재 구현** (106-109행):
```csharp
gyroX = (short)(5.0 * Math.Sin(time * 0.5) * 32768 / 250.0);
gyroY = (short)(5.0 * Math.Sin(time * 0.5 + 1) * 32768 / 250.0);
gyroZ = (short)(5.0 * Math.Sin(time * 0.5 + 2) * 32768 / 250.0);
```

**검증 항목**:
- [ ] **범위**: ±250 dps 설정이 맞는지 확인
- [ ] **스케일 팩터**: 32768 / 250.0 = 131.072 LSB/dps가 올바른지 확인
- [ ] **회전 속도**: 5 dps 시뮬레이션이 현실적인지 확인 (풍선은 느리게 회전)
- [ ] **단위**: raw 값이 16-bit signed integer 범위 내인지 확인
- [ ] **레지스터 주소**: 0x22-0x27이 올바른지 데이터시트 확인

**개선 필요 시**:
- [ ] 스케일 팩터 수정
- [ ] 회전 속도 조정
- [ ] 시뮬레이션 패턴 변경

### 3.2 바이트 순서 (Endianness) 확인

**현재 구현** (112-138행):
```csharp
case 0: return (byte)(accelX & 0xFF);        // Low byte
case 1: return (byte)((accelX >> 8) & 0xFF); // High byte
```

- [ ] **Little-endian** 순서가 올바른지 데이터시트 확인
- [ ] X, Y, Z 축 순서가 올바른지 확인
- [ ] 레지스터 주소 매핑이 정확한지 확인

### 3.3 타이밍 및 샘플링

**현재 구현** (98행):
```csharp
time += 0.001;  // 1ms 증가
```

- [ ] 시간 증가량이 적절한지 확인
- [ ] 센서 ODR (Output Data Rate)과 일치하는지 확인
- [ ] 실제 펌웨어의 샘플링 주기와 호환되는지 확인

---

## 4. 실제 하드웨어와 비교 (선택사항, 강력 권장)

### 4.1 실제 LSM6DSV16X 센서 데이터 수집

- [ ] 실제 하드웨어에서 WHO_AM_I 읽기
- [ ] 정지 상태에서 가속도계 데이터 수집 (중력만 작용)
- [ ] 정지 상태에서 자이로스코프 데이터 수집 (0 근처)
- [ ] 움직임/회전 시 데이터 수집

### 4.2 시뮬레이션 데이터와 비교

- [ ] WHO_AM_I 값 일치 여부
- [ ] 정지 상태 가속도계 Z축 값 비교 (~16384 for +1g)
- [ ] 정지 상태 자이로스코프 값 비교 (~0)
- [ ] 데이터 범위가 현실적인지 확인

### 4.3 차이점 기록 및 수정

**발견된 차이점**:
```
[여기에 실제 하드웨어와 시뮬레이션의 차이점 기록]
예: 가속도계 Z축 오프셋이 실제보다 +500 LSB 높음
```

**필요한 수정 사항**:
```
[여기에 필요한 코드 수정 사항 기록]
예: accelZ 계산식에서 1.0을 0.97로 수정 필요
```

---

## 5. 에러 케이스 처리 검증

### 5.1 I2C 통신 오류

- [ ] 잘못된 레지스터 주소 읽기 시 동작 확인
- [ ] 범위를 벗어난 읽기 시도 처리 확인
- [ ] Write 동작 처리 확인

### 5.2 센서 리셋

Renode 콘솔에서:
```
(monitor) i2c1_lsm6dsv16x Reset
```

- [ ] Reset 후 레지스터가 초기값으로 돌아가는지 확인
- [ ] 로그에 "LSM6DSV16X Reset" 메시지가 나오는지 확인

---

## 6. 문서 검증

### 6.1 README 파일 검토

- [ ] `renode/build_dll/README.md`가 명확하고 이해하기 쉬운지
- [ ] 모든 빌드 단계가 정확한지
- [ ] 문제 해결 섹션이 유용한지

### 6.2 기술 문서 검토

- [ ] `renode/README.md` 업데이트가 적절한지
- [ ] `docs/memory/renode_implementation_progress.md` 기록이 정확한지
- [ ] Phase 4 내용이 완전한지

---

## 7. 최종 승인

### 7.1 통합 테스트

- [ ] DLL 빌드 → 로드 → 실행 전체 플로우가 작동함
- [ ] I2C 센서 통신이 정상적으로 시뮬레이션됨
- [ ] 펌웨어가 센서 데이터를 읽을 수 있음

### 7.2 개선 사항 기록

**즉시 수정 필요**:
```
[여기에 반드시 수정해야 할 사항 기록]
```

**향후 개선 사항**:
```
[여기에 나중에 개선할 사항 기록]
예: MS5611 센서 추가, GPS NMEA 시뮬레이션 등
```

### 7.3 승인 서명

- [ ] 센서 모킹 데이터가 실제 하드웨어/데이터시트와 비교 검증됨
- [ ] 빌드 시스템이 올바르게 작동함
- [ ] 문서가 완전하고 정확함
- [ ] 모든 중요 항목이 체크됨

**검증자**: ____________________
**날짜**: ____________________
**서명**: ____________________

---

## 8. 추가 참고 사항

### 데이터시트 링크

- **LSM6DSV16X**: https://www.st.com/resource/en/datasheet/lsm6dsv16x.pdf
- **MS5611** (향후): https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FMS5611-01BA03%7FB3%7Fpdf
- **SHT31** (향후): https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf

### Renode 문서

- **I2C 페리페럴 작성**: https://renode.readthedocs.io/en/latest/advanced/writing-peripherals.html
- **플랫폼 정의**: https://renode.readthedocs.io/en/latest/advanced/platform-description-format.html

### 연락처

질문이나 문제가 있으면 프로젝트 메인테이너에게 문의하세요.

---

**중요 알림**: 센서 모킹 동작은 반드시 실제 하드웨어/데이터시트와 비교해서 검증해 주세요.
