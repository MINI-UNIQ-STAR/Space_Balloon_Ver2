# PowerShell script to build and run Renode simulation
# Usage: .\renode\run_simulation.ps1

param(
    [switch]$BuildOnly = $false,
    [switch]$SimOnly = $false,
    [string]$BuildType = "Debug"
)

$ErrorActionPreference = "Stop"

Write-Host "======================================" -ForegroundColor Cyan
Write-Host "STM32 Radiosonde Renode Simulation" -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

# Check if Renode is installed
if (-not $SimOnly) {
    Write-Host "[1/3] Checking Renode installation..." -ForegroundColor Yellow
    $renodeCmd = Get-Command renode -ErrorAction SilentlyContinue
    if (-not $renodeCmd) {
        Write-Host "Error: Renode not found in PATH" -ForegroundColor Red
        Write-Host "Please install Renode from https://renode.io/" -ForegroundColor Red
        exit 1
    }
    Write-Host "  Found: $($renodeCmd.Source)" -ForegroundColor Green
}

# Build firmware
if (-not $SimOnly) {
    Write-Host ""
    Write-Host "[2/3] Building firmware ($BuildType)..." -ForegroundColor Yellow

    # Check if cube-cmake is available
    $cubeCmakeCmd = Get-Command cube-cmake -ErrorAction SilentlyContinue
    if (-not $cubeCmakeCmd) {
        Write-Host "Error: cube-cmake not found in PATH" -ForegroundColor Red
        Write-Host "Please install cube-cmake (STM32CubeMX)" -ForegroundColor Red
        exit 1
    }

    # Build
    $buildDir = Join-Path $PWD "build\$BuildType"
    if (-not (Test-Path $buildDir)) {
        Write-Host "  Configuring CMake..." -ForegroundColor Gray
        
        # Tools path
        $makePath = Join-Path $PWD "tools\xpack-windows-build-tools-4.4.1-3\bin\make.exe"
        $toolchain = Join-Path $PWD "cmake\gcc-arm-none-eabi.cmake"
        
        cube-cmake -B $buildDir -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=$BuildType -DCMAKE_MAKE_PROGRAM="$makePath" -DCMAKE_TOOLCHAIN_FILE="$toolchain"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Error: CMake configuration failed" -ForegroundColor Red
            exit 1
        }
    }

    Write-Host "  Building..." -ForegroundColor Gray
    cube-cmake --build $buildDir
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error: Build failed" -ForegroundColor Red
        exit 1
    }

    Write-Host "  Build successful!" -ForegroundColor Green

    # Check if ELF file exists
    $elfPath = "$buildDir\stm32_spaceballoon.elf"
    if (-not (Test-Path $elfPath)) {
        Write-Host "Error: ELF file not found at $elfPath" -ForegroundColor Red
        exit 1
    }
}

if ($BuildOnly) {
    Write-Host ""
    Write-Host "Build-only mode. Exiting." -ForegroundColor Cyan
    exit 0
}

# Run simulation
Write-Host ""
Write-Host "[3/3] Starting Renode simulation..." -ForegroundColor Yellow
Write-Host "  Platform: STM32G431CBU6" -ForegroundColor Gray
    Write-Host "  Script: renode\scripts\simulation.resc" -ForegroundColor Gray
    Write-Host ""
    
    # Create absolute path for simulation script
    $scriptPath = Resolve-Path "renode\scripts\simulation.resc"

Write-Host "Press Ctrl+C in Renode to exit simulation" -ForegroundColor Cyan
Write-Host ""

# Run Renode
renode $scriptPath
