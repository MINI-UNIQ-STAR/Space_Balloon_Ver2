#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- I2C Mock Configuration ---
#define I2C_SDA 6
#define I2C_SCL 7
#define I2C_LSM6DSV16X_ADDR 0x6B
#define LSM6DSV16X_ID 0x70

// Mock Values
float mock_accel[3] = {0,0,1}; // Z-gravity
float mock_gyro[3]  = {0,0,0};

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_accel[0] = pkt->accel[0];
  mock_accel[1] = pkt->accel[1];
  mock_accel[2] = pkt->accel[2];
  mock_gyro[0] = pkt->gyro[0];
  mock_gyro[1] = pkt->gyro[1];
  mock_gyro[2] = pkt->gyro[2];
}

volatile uint8_t current_reg = 0;

void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  uint8_t val = 0;
  
  if (current_reg == 0x0F) { // WHO_AM_I
    val = LSM6DSV16X_ID;
  } 
  else {
    // Return dummy data or mapped accel/gyro
    // Implementing full Register Map (OUTX_L_G, etc.) is tedious but ideal.
    // For now, return 0 to pass init check.
    val = 0x00;
  }
  
  Wire.write(val);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL LSM6DSV16X Mock (ESP-NOW Active)");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(I2C_LSM6DSV16X_ADDR, I2C_SDA, I2C_SCL, 100000)) {
    Serial.println("I2C Slave Init Failed!");
  } else {
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Serial.println("I2C Slave Active (Addr 0x6B)");
  }
}

void loop() {
  delay(10);
}
