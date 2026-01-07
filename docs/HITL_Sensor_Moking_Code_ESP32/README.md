# HITL 센서 에뮬레이터 & GCS (Sensor Sender)

이 디렉토리는 하드웨어-인더-루프(HITL) 시뮬레이션을 위한 **센서 데이터 전송용 GCS 프로그램(`sensor_sender.py`)**과 **ESP32 에뮬레이터 펌웨어(`sensor_emulator.ino`)**를 포함하고 있습니다.

## 🖥️ PC용 GCS 프로그램 (`sensor_sender.py`)

Python 기반의 Grafana 스타일 GUI 프로그램으로, 가상의 센서 데이터를 생성하여 에뮬레이터(ESP32)로 전송합니다.

### 1. 주요 기능
*   **다크 테마 대시보드**: Grafana 스타일의 직관적인 UI.
*   **실시간 그래프**: 고도, 기압, 가속도(IMU), CO2, 방사선 등 센서 데이터 시각화.
*   **궤적 지도 (Trajectory Map)**: GPS 비행 경로 실시간 표시.
*   **3가지 동작 모드**:
    1.  **General Mode (일반 모드)**: 경상국립대(GNU) 좌표 고정 (기본 연결 테스트용).
    2.  **Scenario Mode (시나리오 모드)**: `V4630075.json` 비행 데이터를 리플레이 (실제 비행 시뮬레이션).
    3.  **FDIR Test Mode (고장 주입 모드)**: 정적 데이터에 사용자가 직접 고장(Fault)을 주입하여 테스트.
*   **고장 주입 (Fault Injection)**: 팝업 메뉴를 통해 GPS Timeout, Baro Freeze 등 에러 상황 발생.
*   **MOCK 모드**: 하드웨어 없이 GUI 및 시뮬레이션 로직만 테스트 가능.

### 2. 설치 및 실행

#### 필요 라이브러리 설치
```bash
pip install matplotlib pyserial PySide6
```

#### 프로그램 실행
```bash
python sensor_sender.py
```

### 3. 사용 방법

1.  **연결 (Connection)**:
    *   **COM 포트 선택**: 에뮬레이터(ESP32)가 연결된 포트를 선택하고 `CONNECT`를 클릭합니다.
    *   **MOCK (Test Mode)**: 하드웨어가 없을 경우, 이 옵션을 선택하여 GUI 동작을 확인할 수 있습니다.
2.  **모드 선택**: `Mode` 드롭다운에서 원하는 시뮬레이션 모드를 선택합니다.
3.  **시뮬레이션 시작**: `▶ START` 버튼을 누르면 데이터 전송이 시작됩니다.
4.  **고장 주입 (Fault Injection)**:
    *   `⚠ FAULT MENU` 버튼을 클릭합니다.
    *   원하는 고장 유형(예: "GPS TIMEOUT")을 클릭하면 즉시 적용됩니다.
    *   비행 컴퓨터(Flight Computer)가 해당 고장을 감지하고 FDIR 로직을 수행하는지 확인합니다.

---

## 📡 ESP32 에뮬레이터 펌웨어 (`sensor_emulator.ino`)

PC에서 받은 데이터를 파싱하여 UART를 통해 비행 컴퓨터(LoRa32)로 전달하는 브릿지 역할을 합니다.

*   **업로드**: ESP32 보드에 이 코드를 업로드합니다.
*   **배선 (Wiring)**: `wiring.md` 파일을 참조하여 비행 컴퓨터와 연결합니다.
    *   ESP32(Emulator) -> LoRa32(Flight Computer)
    *   **GPS TX**: GPIO 17 -> GPIO 34
    *   **Aux TX**: GPIO 19 -> GPIO 35
    *   **GND**: 서로 연결 (Common Ground)

## ⚠️ 데이터 포맷

GCS -> 에뮬레이터 -> 비행 컴퓨터로 전송되는 데이터 패킷 형식:

```text
ALL:<lat>,<lon>,<alt>,<press>,<temp>,<co2>,<rad>,<acc_x>,<acc_y>,<acc_z>
```
*   모든 센서 데이터(11종)가 하나의 통합 패킷으로 전송됩니다.
