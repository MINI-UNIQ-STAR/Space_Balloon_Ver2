기존 esp32통신 HITL을 uno r4에 맞게 코드변경 및 test시간을 3초 간격으로 수정한 버전입니다.
배선도 
[브레드보드 신호 줄 1]
R4-MAIN D1/TX
R4-A D0/RX
R4-B D0/RX
R4-C D0/RX
R4-D D0/RX

[브레드보드 신호 줄 1]
R4-MAIN D1/
R4-A D0/RX
R4-B D0/RX
R4-C D0/RX
R4-D D0/RX

STM32 A15 -> R4-A SCL, R4-B SCL
STM32 B9  -> R4-A SDA, R4-B SDA

STM32 A8  -> R4-C SCL, R4-D SCL
STM32 B5  -> R4-C SDA, R4-D SDA

R4-D D4 -> STM32 A10
R4-D D5 -> STM32 A3

[A,B,C,D]는 각각 i2c1 폴더명 순서대로 하면됨 

sensor_sender.py에서 R4-MAIN COM (main uno포트에 맞게)선택
CONNECT
start 및 auto test 시작
자동종료되니 완료되면 끄기 3초로 설정했기때문에 각 항복별로 그래프가 3초씩 멈추는지 확인할것

