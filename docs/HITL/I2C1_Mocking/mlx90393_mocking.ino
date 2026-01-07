#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- MLX90393 Mock Configuration ---
#define I2C_SDA 6
#define I2C_SCL 7
#define MLX90393_ADDR 0x0C

// Mock Values (ESP-NOW)
float mock_mag[3] = {0,0,0};

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_mag[0] = pkt->mag[0];
  mock_mag[1] = pkt->mag[1];
  mock_mag[2] = pkt->mag[2];
}

volatile uint8_t last_cmd = 0;

void onReceive(int len) {
  if (len > 0) {
    last_cmd = Wire.read();
    while (Wire.available()) Wire.read(); 
  }
}

void onRequest() {
  Wire.write(0x00); // Status Byte (Always OK)

  // MLX90393 data read logic is complex (0x4E command to read meas).
  // Driver usually sends RM (0x4E) then reads 7 bytes (Status + 2xX + 2xY + 2xZ)
  // Or Status + T + X + Y + Z...
  // For simplicity, we just return Status. If Master reads more, we send 0s or Data.
  // Note: Wire slave must push data if requested.
  // We can push dummy or real mag data if we track "Last Command was RM".
  // But Driver init is simple. Let's just push 6 more bytes just in case.
  
  // Convert float uT to raw? Sensitivity depends on GAIN.
  // Assuming default gain.
  
  for(int i=0; i<6; i++) Wire.write(0x00);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL MLX90393 Mock (Address 0x0C) [ESP-NOW Active]");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(MLX90393_ADDR, I2C_SDA, I2C_SCL, 100000)) {
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
