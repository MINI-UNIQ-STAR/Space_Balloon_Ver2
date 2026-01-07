#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- CM1107N Mock Configuration ---
// I2C Address: 0x31
#define CM1107N_I2C_ADDR 0x31
#define I2C_SDA 21
#define I2C_SCL 22

// Mock Values
uint16_t mock_co2 = 400;

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_co2 = pkt->co2;
}

// CM1107N Protocol State
// Driver sends 4 bytes: [0x11, 0x01, 0x01, 0xED] (Read CO2)
// Driver reads 8 bytes: [0x16, 0x05, 0x01, DF1, DF2, DF3, DF4, CS]

volatile uint8_t rx_buffer[4];
volatile uint8_t rx_idx = 0;

void onReceive(int len) {
  rx_idx = 0;
  while (Wire.available()) {
    if (rx_idx < 4) {
      rx_buffer[rx_idx++] = Wire.read();
    } else {
      Wire.read(); // Consume excess
    }
  }
}

void onRequest() {
  // Check if last command was Reading CO2 (0x11...)?
  // We mock response regardless of specific command for simplicity
  
  uint8_t resp[8];
  resp[0] = 0x16; // Head
  resp[1] = 0x05; // Length
  resp[2] = 0x01; // Cmd/Type
  
  // CO2 Data (DF1, DF2)
  resp[3] = (mock_co2 >> 8) & 0xFF;
  resp[4] = mock_co2 & 0xFF;
  
  // DF3, DF4 (Status/Reserved?)
  resp[5] = 0x00; 
  resp[6] = 0x00;
  
  // Checksum: (256 - (Sum % 256)) % 256
  // Sum of bytes 0..6
  uint16_t sum = 0;
  for(int i=0; i<7; i++) sum += resp[i];
  resp[7] = (256 - (sum % 256)) % 256;
  
  Wire.write(resp, 8);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL CM1107N Mock (addr 0x31) [ESP-NOW Active]");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(CM1107N_I2C_ADDR)) { 
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
