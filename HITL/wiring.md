# HITL 배선 가이드 (Wiring Guide)

이 문서는 하드웨어-인더-루프(HITL) 테스트를 위해 **ESP32 센서 에뮬레이터**와 **LoRa32 비행 컴퓨터(DUT)**를 연결하는 방법을 설명합니다.

## 개요
에뮬레이터(ESP32)는 가상의 센서 데이터(GPS, 기압, IMU 등)를 생성하여 UART 통신을 통해 비행 컴퓨터(LoRa32)로 전송합니다.

## 핀 연결 (Pin Connections)

### 1. 공통 접지 (Common Ground)
*   **ESP32 GND** <--> **LoRa32 GND**
*   *주의: 안정적인 통신을 위해 반드시 두 보드의 GND를 서로 연결해야 합니다.*

### 2. GPS 에뮬레이션 (UART)
에뮬레이터가 NMEA 문장(GPS 데이터)을 비행 컴퓨터로 전송합니다.

| 신호 (Signal) | ESP32 (에뮬레이터) | LoRa32 (DUT) | 설명 |
| :--- | :--- | :--- | :--- |
| **GPS TX** | **GPIO 17** | **GPIO 34** | 에뮬레이터가 GPS 데이터를 보냅니다. (TX -> RX) |
| **GPS RX** | GPIO 16 | GPIO 12 | (선택 사항) DUT가 GPS에 명령을 보낼 때 사용합니다. |

*   *LoRa32 참고*: GPIO 34번은 입력 전용 핀(Input Only)이므로, UART 수신(RX)용으로 적합합니다.

### 3. Aux/기압 에뮬레이션 (UART)
에뮬레이터가 환경 센서 데이터(통합 패킷)를 비행 컴퓨터로 전송합니다.

| 신호 (Signal) | ESP32 (에뮬레이터) | LoRa32 (DUT) | 설명 |
| :--- | :--- | :--- | :--- |
| **Aux TX** | **GPIO 19** | **GPIO 35** | 에뮬레이터가 기압/IMU 데이터를 보냅니다. (TX -> RX) |
| **Aux RX** | GPIO 18 | GPIO 14 | (선택 사항) DUT가 동기화를 요청할 때 사용합니다. |

*   *LoRa32 참고*: GPIO 35번 또한 입력 전용 핀입니다.

## 전원 공급 (Power)
*   개발 및 모니터링을 위해 두 보드 모두 PC의 USB 포트에 연결하여 전원을 공급합니다.

## 소프트웨어 설정 (DUT 측 코드)
LoRa32 비행 컴퓨터 펌웨어는 아래와 같이 HardwareSerial 핀을 설정해야 합니다:

```c
// DUT (LoRa32) 코드 예시
#include <HardwareSerial.h>

HardwareSerial GPS_Serial(1);
HardwareSerial BARO_Serial(2);

void setup() {
  // GPS 연결: UART1 (RX=34, TX=12)
  GPS_Serial.begin(9600, SERIAL_8N1, 34, 12);

  // 센서 통합 데이터 연결: UART2 (RX=35, TX=14)
  BARO_Serial.begin(115200, SERIAL_8N1, 35, 14);
}
```
