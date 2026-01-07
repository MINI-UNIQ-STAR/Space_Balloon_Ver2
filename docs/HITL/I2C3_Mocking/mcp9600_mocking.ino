#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- MCP9600 Mock Configuration ---
// ST Driver uses 0x67 (Default)
#define MCP9600_ADDR 0x67 
#define I2C_SDA 21
#define I2C_SCL 22

// Mock Values
float mock_ext_temp = -50.0; // Stratosphere cold
float mock_amb_temp = 20.0;  // Internal

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_ext_temp = pkt->ext_temp_c;
  // Ambient could be board temp
  mock_amb_temp = pkt->temp_c;
}

volatile uint8_t last_reg = 0;

void onReceive(int len) {
  if (len > 0) {
    last_reg = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  // Registers: 
  // 0x00: Hot Junction (Thermocouple)
  // 0x01: Delta
  // 0x02: Cold Junction (Ambient)
  
  float target = 0.0;
  if (last_reg == 0x00) target = mock_ext_temp;
  else if (last_reg == 0x02) target = mock_amb_temp;
  else target = 0.0; // Default
  
  // Format: int16_t value = T / 0.0625
  int16_t val = (int16_t)(target / 0.0625f);
  
  // Big Endian? Buffer[0] is MSB
  Wire.write((val >> 8) & 0xFF);
  Wire.write(val & 0xFF);
}

void setup() {
  Serial.begin(115200);
  Serial.printf("HITL MCP9600 Mock (addr 0x%02X) [ESP-NOW Active]\n", MCP9600_ADDR);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(MCP9600_ADDR)) {
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
