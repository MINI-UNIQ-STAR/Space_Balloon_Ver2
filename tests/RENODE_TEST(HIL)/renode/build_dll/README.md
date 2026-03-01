# Renode 센서/페리페럴 DLL 빌드

이 디렉토리는 STM32G431 Renode 시뮬레이션용 센서 및 페리페럴 C# 코드를 DLL로 컴파일하기 위한 빌드 시스템입니다.

## 빌드 요구사항

- .NET 6.0 SDK 이상
- Renode 설치 (DLL 참조용)
- PowerShell (Windows) 또는 pwsh (Linux/macOS)

## 빌드 방법

### Windows (PowerShell)

```powershell
cd renode/build_dll
.\build.ps1
```

### Renode 경로 지정

Renode가 표준 위치에 설치되지 않은 경우:

```powershell
.\build.ps1 -RenodePath "C:\CustomPath\Renode\bin"
```

### 환경 변수 사용

```powershell
$env:RENODE_ROOT = "C:\CustomPath\Renode\bin"
.\build.ps1
```

## 출력

빌드가 성공하면 다음 파일이 생성됩니다:

```
renode/build_dll/bin/Release/SensorPeripherals.dll
```

이 DLL에는 다음 컴포넌트들이 포함됩니다:

### 센서 (sensors/)
- `LSM6DSV16X` - 6축 IMU 센서 (가속도계 + 자이로스코프)
- `GPSSimulator` - GPS XA1110 NMEA 시뮬레이터 (UART)

### 페리페럴 (peripherals/)
- `STM32G4_RCC` - Reset and Clock Control
- `STM32G4_FLASH` - Flash 컨트롤러
- `STM32G4_PWR` - Power 컨트롤

## Renode에서 사용하기

### 방법 1: 테스트 스크립트 사용

```bash
cd renode
renode --disable-xwt --console test_sensor_dll.resc
```

### 방법 2: 수동 로드

Renode 콘솔에서:

```
(monitor) machine LoadPeripheralsAssembly @renode/build_dll/bin/Release/SensorPeripherals.dll
(monitor) machine LoadPlatformDescription @renode/stm32g431_with_sensors.repl
```

### 방법 3: .repl 파일에서 직접 로드

```repl
using "renode/build_dll/bin/Release/SensorPeripherals.dll"

i2c1:
    lsm6dsv16x: Sensors.LSM6DSV16X @ 0x6B
```

## 문제 해결

### "Renode DLL을 찾을 수 없습니다"

Renode 설치 경로를 확인하고 `-RenodePath` 옵션을 사용하세요:

```powershell
.\build.ps1 -RenodePath "정확한\Renode\bin\경로"
```

### "dotnet SDK가 설치되어 있지 않습니다"

https://dotnet.microsoft.com/download 에서 .NET 6.0 SDK 이상을 설치하세요.

### "빌드 실패"

1. C# 소스 파일 확인:
   - `../sensors/*.cs`
   - `../peripherals/*.cs`

2. Renode DLL 참조 확인:
   - `Antmicro.Renode.Core.dll`
   - `Antmicro.Renode.Peripherals.dll`
   - `Antmicro.Renode.Logging.dll`
   - `Antmicro.Renode.Utilities.dll`

## 개발자 노트

### 새 센서/페리페럴 추가

1. `../sensors/` 또는 `../peripherals/`에 C# 파일 추가
2. `SensorPeripherals.csproj`의 `<Compile Include="...">` 항목에 추가
3. `.\build.ps1` 실행

### 네임스페이스 규칙

- 센서: `Antmicro.Renode.Peripherals.Sensors`
- 페리페럴: `Antmicro.Renode.Peripherals.Miscellaneous`
- UART 장치: `Antmicro.Renode.Peripherals.UART`

## 참고 자료

- [Renode 공식 문서](https://renode.readthedocs.io/)
- [C# 페리페럴 작성 가이드](https://renode.readthedocs.io/en/latest/advanced/writing-peripherals.html)
- [프로젝트 메인 README](../../README.md)
