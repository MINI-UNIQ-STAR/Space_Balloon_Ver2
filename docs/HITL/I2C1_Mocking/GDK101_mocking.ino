#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- GDK101 Mock Configuration ---
#define I2C_SDA 6
#define I2C_SCL 7
#define GDK101_ADDR 0x18

// Mock Values
float mock_rad = 0.0f;

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_rad = pkt->radiation;
}

volatile uint8_t current_reg = 0;

void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  // Return Status/Vibration or Data?
  // Minimally just 0s or Data based on mock_rad.
  // GDK101 output format (example): 2 bytes value? 
  // Assume simple read.
  
  uint16_t val = (uint16_t)(mock_rad * 100.0f); // just a scale
  Wire.write(val & 0xFF);
  Wire.write((val >> 8) & 0xFF);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL GDK101 Mock (Address 0x18) [ESP-NOW Active]");
  
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(GDK101_ADDR, I2C_SDA, I2C_SCL, 100000)) {
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
