#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- SHT31 Mock Configuration ---
#define SHT31_ADDR 0x44

// Mock Values (Global, updated via ESP-NOW)
float mock_temp = 25.0;
float mock_hum = 50.0;

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_temp = pkt->temp_c;
  mock_hum = pkt->humidity;
}

volatile uint8_t last_msb = 0;
volatile uint8_t last_lsb = 0;

uint8_t crc8(const uint8_t *data, int len) {
  uint8_t crc = 0xFF;
  for (int j = 0; j < len; j++) {
      crc ^= data[j];
      for (int i = 0; i < 8; i++) {
          crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
      }
  }
  return crc;
}

void onReceive(int len) {
  if (len >= 2) {
    last_msb = Wire.read(); // MSB
    last_lsb = Wire.read(); // LSB 
  }
  while (Wire.available()) Wire.read();
}

void onRequest() {
  // Return 6 bytes: [TempMSB, TempLSB, CRC, HumMSB, HumLSB, CRC]
  
  // Convert Values
  // T val = (T + 45) * 65535 / 175
  uint16_t t_raw = (uint16_t)((mock_temp + 45.0f) * 65535.0f / 175.0f);
  // H val = H * 65535 / 100
  uint16_t h_raw = (uint16_t)(mock_hum * 65535.0f / 100.0f);
  
  uint8_t buf[2];
  
  // Temp
  buf[0] = (t_raw >> 8) & 0xFF; 
  buf[1] = t_raw & 0xFF;
  Wire.write(buf[0]);
  Wire.write(buf[1]);
  Wire.write(crc8(buf, 2));
  
  // Hum
  buf[0] = (h_raw >> 8) & 0xFF;
  buf[1] = h_raw & 0xFF;
  Wire.write(buf[0]);
  Wire.write(buf[1]);
  Wire.write(crc8(buf, 2));
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL SHT31 Mock (Address 0x44) [ESP-NOW Active]");

  // Init Wifi & ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(SHT31_ADDR)) {
    Serial.println("I2C Init Failed");
  } else {
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Serial.println("I2C Slave Active");
  }
}

void loop() {
  delay(10);
}
