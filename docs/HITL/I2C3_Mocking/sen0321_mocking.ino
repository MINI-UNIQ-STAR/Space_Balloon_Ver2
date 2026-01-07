#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- SEN0321 (Ozone) Mock Configuration ---
#define I2C_ADDR_SEN0321 0x70
#define I2C_SDA 21
#define I2C_SCL 22

// Mock Values
int16_t mock_ozone = 20;

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_ozone = pkt->ozone_ppb;
}

// Registers
#define REG_MODE 0x03
#define REG_AUTO_DATA_H 0x09
#define REG_AUTO_DATA_L 0x0A

volatile uint8_t current_reg = 0;

void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  if (current_reg == REG_AUTO_DATA_H) {
    // Return Ozone PPB
    Wire.write((mock_ozone >> 8) & 0xFF); // High Byte
    Wire.write(mock_ozone & 0xFF);        // Low Byte
  }
  else if (current_reg == REG_MODE) {
    Wire.write(0x00);
  }
  else {
    Wire.write(0x00); 
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL SEN0321 (Ozone) Mock (Address 0x70) [ESP-NOW Active]");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  // Note: Use default pins or custom
  if (!Wire.begin(I2C_ADDR_SEN0321)) { 
    Serial.println("I2C Init Failed");
  } else {
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Serial.println("I2C Slave Active (0x70)");
  }
}

void loop() {
  delay(10);
}
