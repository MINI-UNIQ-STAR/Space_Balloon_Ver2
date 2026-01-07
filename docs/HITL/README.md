# HITL 시뮬레이션 시스템 (5-Board 최적화 구성)

이 디렉토리는 스페이스발룬 프로젝트의 **HITL (Hardware-In-The-Loop)** 시뮬레이션을 위한 통합 펌웨어와 문서를 포함합니다.

## 🏗️ 5-Board 시스템 구성 (Dual-I2C 적용)

총 5개의 ESP32/LoRa32 보드를 사용하여 STM32의 모든 주변장치를 모사합니다. 각 보드는 ESP-NOW로 PC와 통신하며, Dual I2C 기법을 통해 1개의 보드가 2개의 센서 역할을 수행합니다.

### 1. Hub Node (메인 컨트롤)
*   **보드 권장**: **ESP32-C3** (또는 일반 ESP32)
*   **역할**: PC 통신, 텔레메트리 중계, ESP-NOW 브로드캐스트.
*   **펌웨어**: `main_control/main_control.ino`

### 2. Mock Node A (I2C1 주요 센서)
*   **보드 권장**: **LoRa32 #1**
*   **펌웨어**: `I2C1_Mocking/I2C1_Dual_Mock.ino`
*   **연결**:
    *   **I2C Port 0** (`SDA=21`, `SCL=22`) <--> **LSM6DSV16X** (Addr 0x6B) 모사
    *   **I2C Port 1** (`SDA=13`, `SCL=12`) <--> **MLX90393** (Addr 0x0C) 모사
    *   *주의: STM32의 I2C1 라인에 두 포트 모두 병렬 연결.*

### 3. Mock Node B (I2C1 방사선 + GPIO)
*   **보드 권장**: **ESP32 Standard** (GPIO 핀 다수 필요)
*   **펌웨어**: `I2C1_Mocking/I2C1_GDK_GPIO_Mock.ino`
*   **연결**:
    *   **I2C Port 0** (`SDA=21`, `SCL=22`) <--> **GDK101** (Addr 0x18)
    *   **OneWire**: `GPIO 4` <--> **DS18B20** 모사
    *   **DAC**: `GPIO 25` <--> **배터리 ADC**
    *   **PWM In**: `GPIO 18, 19` <--> **히터 제어**
    *   **Resets**: 각종 리셋 핀 연결.

### 4. Mock Node C (I2C3 환경 센서)
*   **보드 권장**: **LoRa32 #2**
*   **펌웨어**: `I2C3_Mocking/I2C3_Dual_Mock_A.ino`
*   **연결**:
    *   **I2C Port 0** (`SDA=21`, `SCL=22`) <--> **MS5611** (Addr 0x77)
    *   **I2C Port 1** (`SDA=13`, `SCL=12`) <--> **SHT31** (Addr 0x44)

### 5. Mock Node D (I2C3 공기질 + UART)
*   **보드 권장**: **LoRa32 #3**
*   **펌웨어**: `I2C3_Mocking/I2C3_Dual_Mock_B_UART.ino`
*   **연결**:
    *   **I2C Port 0** (`SDA=21`, `SCL=22`) <--> **CM1107N** (Addr 0x31)
    *   **I2C Port 1** (`SDA=32`, `SCL=33`) <--> **MCP9600** (Addr 0x60) (핀 번호 코드 확인 필요)
    *   **UART1** (`TX=17, RX=16`) <--> **GPS (XA1110)**
    *   **UART2** (`TX=4, RX=15`) <--> **PMS3003** (핀 번호 코드 확인 필요)

---

## ⚡ 배선 주의사항 (Parallel Wiring)
Dual I2C 모드에서는 하나의 보드에서 나온 두 쌍의 SDA/SCL을 STM32의 같은 버스에 **병렬로** 연결합니다.
(Open-Drain 방식이므로 전기적으로 안전합니다.)

## 🚀 사용법
1.  각 보드에 해당 펌웨어를 업로드합니다.
2.  PC 앱(`sensor_sender.py`) 실행 후 Hub(C3)와 연결합니다.
3.  시뮬레이션 시작 시 모든 위성 노드가 동기화되어 동작합니다.

## 🔎 연결 확인 (Connection Verification)
*   **Heartbeat LED**: 각 Mock 노드는 Main Control로부터 패킷을 수신할 때마다 **내장 LED (GPIO 2)**를 토글(깜빡임)합니다.
*   LED가 빠르게 깜빡인다면(10Hz), Main Control과의 무선 연결이 정상적으로 수립된 것입니다.
