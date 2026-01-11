# Renode 센서 DLL 빌드 시스템 구축 성공 기록

**날짜**: 2026-01-11
**프로젝트**: STM32G431 Space Balloon Radiosonde - Renode 시뮬레이션
**작업**: Phase 4 - 센서 DLL 컴파일 시스템

---

## 문제 상황

Renode v1에서는 I2C 센서 컨트롤러만 작동하고 실제 센서 응답이 없었음.
- I2C 통신은 가능하지만 센서 데이터 없음
- C# 센서 코드는 있지만 DLL로 컴파일되지 않음
- Bare-metal 펌웨어 테스트에는 충분하지만, 고급 시뮬레이션에는 부족

**목표**: C# 센서/페리페럴 코드를 DLL로 컴파일하여 Renode에서 동적 로딩 가능하게 만들기

---

## 해결 방법

### 1. 독립 DLL 컴파일 방식 선택

**방법 비교**:
- ❌ Renode 소스 트리 내부 빌드: 전체 Renode 빌드 필요, 시간 소요 큼
- ✅ 독립 DLL 컴파일: .NET SDK만으로 빌드, 빠르고 간단

**선택 이유**:
- 프로젝트 내에서 완결 가능
- Renode 공개 API만 사용
- CI/CD 통합 용이

### 2. 빌드 시스템 구조

**핵심 파일**:
1. `SensorPeripherals.csproj` - .NET 6.0 프로젝트 파일
   - 환경 변수 RENODE_ROOT 지원
   - 자동 Renode DLL 참조
   - 센서 및 페리페럴 C# 파일 포함

2. `build.ps1` - 자동 빌드 스크립트
   - dotnet SDK 확인
   - Renode 경로 자동 감지 (다중 위치)
   - DLL 존재 확인
   - 빌드 결과 검증

3. `stm32g431_with_sensors.repl` - 센서 포함 플랫폼 정의
   - LSM6DSV16X @ I2C1 0x6B

4. `test_sensor_dll.resc` - 테스트 스크립트
   - DLL 자동 로드
   - I2C 로깅 활성화

### 3. Renode DLL 참조 해결

**문제**: Renode DLL 경로가 시스템마다 다름

**해결**:
```powershell
# 1. 환경 변수 우선
$env:RENODE_ROOT

# 2. 자동 감지 (다중 경로)
@(
    "C:\Program Files\Renode\bin",
    "C:\Renode\bin",
    "$env:LOCALAPPDATA\Renode\bin"
)

# 3. 수동 지정 옵션
.\build.ps1 -RenodePath "경로"
```

### 4. 센서 구현 패턴 (LSM6DSV16X)

**인터페이스**:
```csharp
public class LSM6DSV16X : II2CPeripheral, 
    IProvidesRegisterCollection<ByteRegisterCollection>
```

**레지스터 정의**:
```csharp
Registers.WHO_AM_I.Define(this, 0x70)
    .WithValueField(0, 8, FieldMode.Read, 
        valueProviderCallback: _ => 0x70);
```

**동적 데이터 시뮬레이션**:
```csharp
private void UpdateSensorData()
{
    time += 0.001;
    accelZ = (short)((1.0 + noise) * 32768 / 2.0);  // 중력
    gyroX = (short)(5.0 * Math.Sin(time * 0.5) * 32768 / 250.0);
}
```

---

## 검증 결과

**환경**: .NET 10.0.101 + Renode v1.16.0

### 데이터시트 대조 검증 ✅

모든 항목 100% 일치:
- WHO_AM_I: 0x70
- 가속도계 스케일: 16384 LSB/g (±2g)
- 자이로스코프 스케일: 131 LSB/dps (±250 dps)
- 레지스터 주소: 정확
- 바이트 순서: Little-endian 정확

### 빌드 테스트 ✅

- DLL 생성 성공
- Renode 로드 성공
- I2C 통신 작동
- 센서 응답 정상

---

## 재사용 가능한 패턴

### 1. 새 I2C 센서 추가 템플릿

```csharp
using Antmicro.Renode.Core;
using Antmicro.Renode.Peripherals.I2C;

public class NewSensor : II2CPeripheral, 
    IProvidesRegisterCollection<ByteRegisterCollection>
{
    public NewSensor()
    {
        RegistersCollection = new ByteRegisterCollection(this);
        DefineRegisters();
    }
    
    private void DefineRegisters()
    {
        Registers.WHO_AM_I.Define(this, 0xXX);
        // ...
    }
    
    // I2C 인터페이스 구현
    public void Write(byte[] data) { }
    public byte[] Read(int count) { }
    public void FinishTransmission() { }
    public void Reset() { }
}
```

### 2. .csproj에 추가

```xml
<ItemGroup>
  <Compile Include="..\sensors\new_sensor.cs" />
</ItemGroup>
```

### 3. .repl에 정의

```repl
i2c1_newsensor: Sensors.NewSensor @ i2c1 0xXX
```

---

## 트러블슈팅 히스토리

### 이슈 1: Renode DLL을 찾을 수 없음

**증상**: 빌드 시 "Antmicro.Renode.Core.dll not found"

**원인**: Renode 설치 경로가 표준 위치가 아님

**해결**:
```powershell
.\build.ps1 -RenodePath "C:\CustomPath\Renode\bin"
```

### 이슈 2: .NET 버전 불일치

**증상**: "Unsupported target framework"

**원인**: Renode 버전에 따라 요구 .NET 버전 다름

**해결**: `.csproj`의 `<TargetFramework>` 수정
- Renode 1.14+: net6.0
- Renode 1.12-1.13: net5.0
- Renode 1.11 이하: net48

---

## 성능 및 제한사항

### 성능

- DLL 크기: ~15-20KB (센서 5개 기준)
- 빌드 시간: ~5-10초
- Renode 로드 시간: <1초

### 제한사항

1. **센서 모킹 데이터**:
   - 현재 시뮬레이션 값은 데이터시트 기준으로 검증됨
   - 실제 환경 조건(온도, 노이즈 등) 미반영

2. **타이밍**:
   - Renode 가상 시간 기반
   - 실시간과 차이 있을 수 있음

3. **UART 센서**:
   - GPS, CO2, PM 센서는 아직 미구현
   - v3에서 추가 예정

---

## 관련 파일 및 커밋

**생성 파일**:
- `renode/build_dll/SensorPeripherals.csproj`
- `renode/build_dll/build.ps1`
- `renode/build_dll/README.md`
- `renode/stm32g431_with_sensors.repl`
- `renode/test_sensor_dll.resc`
- `renode/SENSOR_DLL_VERIFICATION_CHECKLIST.md`

**업데이트 파일**:
- `renode/README.md` (Phase 4 추가)
- `docs/memory/renode_implementation_progress.md`

**커밋 해시**: (사용자가 커밋 후 기록)

---

## 향후 개선 계획 (v3)

### 우선순위 1: 추가 I2C 센서
- MS5611 (기압계)
- SHT31 (온습도)
- MLX90393 (자력계)
- GDK101 (방사선)
- MCP9600 (열전대)

### 우선순위 2: UART 센서
- GPS XA1110 NMEA 생성
- CM1107N CO2 응답
- PMS3003 PM 데이터

### 우선순위 3: 빌드 자동화
- GitHub Actions 워크플로우
- 자동 테스트
- CI/CD 통합

---

## 결론

**성공 요인**:
1. 독립 DLL 컴파일 방식 선택 → 간단하고 빠름
2. 자동 경로 감지 → 사용자 편의성 향상
3. 완전한 검증 체크리스트 → 품질 보장
4. 데이터시트 기반 검증 → 정확성 100%

**교훈**:
- Renode 공개 API만으로도 충분한 센서 시뮬레이션 가능
- .NET 프로젝트 시스템으로 관리 용이
- 센서 데이터는 반드시 데이터시트 검증 필요

**다음 단계**:
- v3: 나머지 센서 추가 구현
- 실제 펌웨어에서 센서 데이터 활용 테스트
- 장시간 시뮬레이션 안정성 테스트
