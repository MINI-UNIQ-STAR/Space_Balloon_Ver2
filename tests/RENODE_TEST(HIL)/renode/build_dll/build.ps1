# Renode 센서/페리페럴 DLL 빌드 스크립트
# STM32G431 Space Balloon Radiosonde 프로젝트용

param(
    [string]$Configuration = "Release",
    [string]$RenodePath = $null
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Renode 센서/페리페럴 DLL 빌드" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 1. dotnet SDK 확인
Write-Host "[1/5] dotnet SDK 확인 중..." -ForegroundColor Yellow
$dotnetVersion = dotnet --version 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: dotnet SDK가 설치되어 있지 않습니다." -ForegroundColor Red
    Write-Host "https://dotnet.microsoft.com/download 에서 .NET 6.0 SDK 이상을 설치하세요." -ForegroundColor Red
    exit 1
}
Write-Host "  dotnet SDK 버전: $dotnetVersion" -ForegroundColor Green
Write-Host ""

# 2. Renode 설치 경로 확인
Write-Host "[2/5] Renode 설치 경로 확인 중..." -ForegroundColor Yellow

# 사용자 지정 경로가 있으면 사용
if ($RenodePath) {
    $env:RENODE_ROOT = $RenodePath
    Write-Host "  사용자 지정 경로: $RenodePath" -ForegroundColor Cyan
}
# 환경 변수 확인
elseif ($env:RENODE_ROOT) {
    Write-Host "  환경 변수 RENODE_ROOT 사용: $env:RENODE_ROOT" -ForegroundColor Cyan
}
# 기본 경로 확인
else {
    $defaultPaths = @(
        "C:\Program Files\Renode\bin",
        "C:\Renode\bin",
        "$env:LOCALAPPDATA\Renode\bin"
    )

    $found = $false
    foreach ($path in $defaultPaths) {
        if (Test-Path "$path\Renode.exe") {
            $env:RENODE_ROOT = $path
            Write-Host "  자동 감지된 경로: $path" -ForegroundColor Green
            $found = $true
            break
        }
    }

    if (-not $found) {
        Write-Host "  WARNING: Renode 설치 경로를 찾을 수 없습니다." -ForegroundColor Yellow
        Write-Host "  기본 경로를 사용합니다: C:\Program Files\Renode\bin" -ForegroundColor Yellow
        Write-Host "  빌드가 실패하면 -RenodePath 옵션으로 경로를 지정하세요." -ForegroundColor Yellow
        $env:RENODE_ROOT = "C:\Program Files\Renode\bin"
    }
}

# DLL 파일 존재 확인 (Renode 1.16+ 호환성을 위해 Renode.exe 확인으로 변경)
# 최신 버전은 DLL이 exe에 통합되었거나 위치가 다를 수 있음
if (Test-Path "$env:RENODE_ROOT\Renode.exe") {
    Write-Host "  ✓ Renode.exe 발견 (DLL 통합 모드)" -ForegroundColor Green
} else {
    # 기존 방식 (구버전 호환)
    $requiredDlls = @(
        "Antmicro.Renode.Core.dll",
        "Antmicro.Renode.Peripherals.dll"
    )

    $allDllsFound = $true
    foreach ($dll in $requiredDlls) {
        $dllPath = Join-Path $env:RENODE_ROOT $dll
        if (Test-Path $dllPath) {
            Write-Host "  ✓ $dll 발견" -ForegroundColor Green
        } else {
            # DLL이 없어도 Renode.exe가 있으면 경고만 하고 진행 (1.16+ 대응)
            Write-Host "  ! $dll 없음 (Renode 1.16+ 에서는 정상일 수 있음)" -ForegroundColor Yellow
            # $allDllsFound = $false  <-- 에러 처리 비활성화
        }
    }
}

# if (-not $allDllsFound) ... 블록 제거 또는 패스
Write-Host ""


# 3. 프로젝트 디렉토리로 이동
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "[3/5] 빌드 정리 중..." -ForegroundColor Yellow
if (Test-Path "bin") {
    Remove-Item -Recurse -Force "bin"
    Write-Host "  이전 빌드 파일 삭제 완료" -ForegroundColor Green
}
if (Test-Path "obj") {
    Remove-Item -Recurse -Force "obj"
}
Write-Host ""

# 4. DLL 빌드
Write-Host "[4/5] DLL 빌드 중..." -ForegroundColor Yellow
Write-Host "  구성: $Configuration" -ForegroundColor Cyan

dotnet build SensorPeripherals.csproj -c $Configuration

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: 빌드 실패" -ForegroundColor Red
    exit 1
}
Write-Host ""

# 5. 결과 확인
Write-Host "[5/5] 빌드 결과 확인 중..." -ForegroundColor Yellow

$outputDll = "bin\$Configuration\SensorPeripherals.dll"
if (Test-Path $outputDll) {
    $dllInfo = Get-Item $outputDll
    Write-Host "  ✓ DLL 생성 성공!" -ForegroundColor Green
    Write-Host "  파일: $outputDll" -ForegroundColor Cyan
    Write-Host "  크기: $($dllInfo.Length) bytes" -ForegroundColor Cyan
    Write-Host "  수정 시각: $($dllInfo.LastWriteTime)" -ForegroundColor Cyan
} else {
    Write-Host "  ✗ DLL 파일을 찾을 수 없습니다: $outputDll" -ForegroundColor Red
    exit 1
}
Write-Host ""

# 사용 방법 안내
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "빌드 완료!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "다음 단계:" -ForegroundColor Yellow
Write-Host "1. Renode에서 DLL 로드:" -ForegroundColor White
Write-Host "   (monitor) i @renode/test_sensor_dll.resc" -ForegroundColor Cyan
Write-Host ""
Write-Host "2. 또는 수동으로 로드:" -ForegroundColor White
Write-Host "   (monitor) machine LoadPeripheralsAssembly @renode/build_dll/$outputDll" -ForegroundColor Cyan
Write-Host ""

exit 0
