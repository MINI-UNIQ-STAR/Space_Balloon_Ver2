#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Configuration: Node C (LoRa32 #2) ---
// I2C Port 0 (Wire) -> MS5611
#define I2C0_SDA 21
#define I2C0_SCL 22
#define MS_ADDR  0x77

// I2C Port 1 (Wire1) -> SHT31
#define I2C1_SDA 13
#define I2C1_SCL 12
#define SHT_ADDR 0x44

#define LED_PIN 2

// Mock State
volatile float temp = 25.0;
volatile float press = 101325.0;
volatile float humid = 50.0;

// --- ESP-NOW Handler ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  temp = pkt->temp_c;
  press = pkt->pressure_pa;
  humid = pkt->humidity;

  // LED Heartbeat
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// --- Wire 0 Logic (MS5611) ---
volatile uint8_t ms_byte = 0;
void onReceive0(int len) {
  if (len > 0) {
    ms_byte = Wire.read();
    while (Wire.available()) Wire.read();
  }
}
void onRequest0() {
  // Simple Mock: Return 3 bytes (0x00,0x00,0x00) for ADC read (0x00)
  // or PROM data for 0xA0..0xAE
  if (ms_byte == 0x00) {
      // ADC Read
      Wire.write(0x80); Wire.write(0x00); Wire.write(0x00);
  } else if (ms_byte >= 0xA0) {
      // PROM Read (Calibration)
      Wire.write(0x00); Wire.write(0x00); 
  }
}

// --- Wire 1 Logic (SHT31) ---
void onReceive1(int len) {
  while(Wire1.available()) Wire1.read();
}
void onRequest1() {
  // SHT31 Data: 6 bytes (Temp MSB, LSB, CRC, Hum MSB, LSB, CRC)
  // Just send dummy 6 bytes to prevent timeouts
  uint8_t buf[6] = {0x66, 0x66, 0x00, 0x88, 0x88, 0x00};
  Wire1.write(buf, 6);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL Node C: I2C3 Dual Mock A (MS5611+SHT31)");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) esp_now_register_recv_cb(OnDataRecv);

  if (!Wire.begin(MS_ADDR, I2C0_SDA, I2C0_SCL, 100000)) Serial.println("Wire(0) Fail");
  else { Wire.onReceive(onReceive0); Wire.onRequest(onRequest0); }

  if (!Wire1.begin(SHT_ADDR, I2C1_SDA, I2C1_SCL, 100000)) Serial.println("Wire(1) Fail");
  else { Wire1.onReceive(onReceive1); Wire1.onRequest(onRequest1); }
  
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  delay(10);
}
