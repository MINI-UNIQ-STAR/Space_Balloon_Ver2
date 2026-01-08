#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Configuration: Node B (ESP32 Standard) ---
// I2C Port 0 (Wire) -> GDK101
#define I2C_SDA 21
#define I2C_SCL 22
#define GDK_ADDR 0x18

// --- GPIOs (ESP32 DevKit V1) ---
// 1. OneWire (Mock DS18B20)
#define ONE_WIRE_PIN 4

// 2. Battery DAC
#define BAT_DAC_PIN 25

// 3. Heaters (PWM In)
#define HEATER_BAT_PIN 18 
#define HEATER_BD_PIN  19

// 4. Resets
// Use available pins. Standard ESP32 has plenty.
#define RST_XA1110_PIN 23 
#define SET_PMS_PIN    26 
#define RST_GDK_PIN    27 
#define RST_SEN_PIN    14 
#define RST_CM_PIN     12 
#define RST_MCP_PIN    13 
#define RST_MS_PIN     32 
#define RST_SHT_PIN    33 
#define RST_LSM_PIN    34 // Input Only OK
#define RST_MLX_PIN    35 // Input Only OK

#define LED_PIN 2

// Mock State
float gdk_rad = 0.01; // uSv/h
float mock_bat = 16000.0; // mV
float t1 = 25.0, t2 = 30.0; // OneWire Temps

// --- ESP-NOW Handler ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  gdk_rad = pkt->radiation;
  mock_bat = pkt->bat_mv;
  t1 = pkt->temp_c;
  
  // DAC Logic
  uint16_t dac_mv = mock_bat / 6; 
  if (dac_mv > 3300) dac_mv = 3300;
  uint8_t dac_val = map(dac_mv, 0, 3300, 0, 255);
  dacWrite(BAT_DAC_PIN, dac_val);

  // LED Heartbeat
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// --- Wire Logic (GDK101) ---
void onRequest() {
  // Return dummy data (2 bytes?)
  // GDK101 logic from driver?
  Wire.write(0x00);
}

// --- OneWire Mock (BitBang) ---
void runOneWireMock() {
  // (Paste simplified logic or include header)
  // For brevity, assuming user uses valid library or previous logic.
  // Re-implementing simplified presence pulse here for robustness:
  if (digitalRead(ONE_WIRE_PIN) == LOW) {
      uint32_t start = micros();
      while(digitalRead(ONE_WIRE_PIN) == LOW);
      if (micros() - start > 400) { // Reset Pulse Detected
          delayMicroseconds(30);
          pinMode(ONE_WIRE_PIN, OUTPUT);
          digitalWrite(ONE_WIRE_PIN, LOW); // Presence
          delayMicroseconds(120);
          digitalWrite(ONE_WIRE_PIN, HIGH);
          pinMode(ONE_WIRE_PIN, INPUT);
      }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL Node B: GDK + GPIO Mock");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) esp_now_register_recv_cb(OnDataRecv);

  // Init I2C for GDK
  Wire.begin(GDK_ADDR, I2C_SDA, I2C_SCL, 100000);
  Wire.onRequest(onRequest);

  // Init GPIOs
  pinMode(ONE_WIRE_PIN, INPUT); // PULLUP external
  pinMode(HEATER_BAT_PIN, INPUT);
  pinMode(HEATER_BD_PIN, INPUT);
  
  uint8_t rst_pins[] = {23, 26, 27, 14, 12, 13, 32, 33, 34, 35};
  for(int i=0; i<10; i++) pinMode(rst_pins[i], INPUT_PULLUP);
  
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  runOneWireMock();
  // Check resets/heaters logic
  delay(1);
}
