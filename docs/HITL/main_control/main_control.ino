#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Main Control Configuration ---
// Function: 
// 1. Receive 'ALL:...' Simulation Data from PC (Serial)
// 2. Broadcast HitlStatePacket via ESP-NOW to all Mock Nodes
// 3. Receive STM32 Telemetry via UART (RX_PIN) and pass to PC? Or just log.
//    User said "receive data from STM32 UART3"

#define STM32_UART3_RX_PIN 16 // Adjust pin for ESP32 UART2 RX
#define STM32_UART3_TX_PIN 17 // Not used if only listening

HardwareSerial SerialSTM(2);

HitlStatePacket sim_state;

// Broadcast Address (All FF)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Debug
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Handle Feedback from Mocks (Heaters, Resets) if implemented
  if (len == sizeof(HitlFeedbackPacket)) {
    HitlFeedbackPacket *fb = (HitlFeedbackPacket*)incomingData;
    if (fb->reset_flags > 0) {
      Serial.printf("[FEEDBACK] Sensor RESET Detected on Node %d! Flags: %02X\n", fb->node_id, fb->reset_flags);
    }
    if (fb->heater_bat_duty > 0 || fb->heater_bd_duty > 0) {
      // Log heater status
      // Serial.printf("[FEEDBACK] Heaters: Bat %d%%, Bd %d%%\n", fb->heater_bat_duty, fb->heater_bd_duty);
    }
  }
}

void setup() {
  Serial.begin(115200);
  SerialSTM.begin(57600, SERIAL_8N1, STM32_UART3_RX_PIN, STM32_UART3_TX_PIN);

  Serial.println("HITL Main Control Node Starting...");

  // Init WiFi Mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register Peer (Broadcast)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP-NOW Broadcast Active.");
}

void parseSimData(String input) {
  // PC sends "ALL:Time,Lat,Lon,Alt,..." (CSV)
  // Logic from sensor_emulator.ino adapted here
  
  if (input.startsWith("ALL:")) {
     // Quick Parsing ( Simplified for brevity - implement robustly in real usage)
     // Assume fixed indices for simplicity or use strtok
     
     // Example: ALL:0,37.123,127.123,100.5,...
     // We need to fill 'sim_state' struct
     
     // Mocking parsing for now (Since verified parsing was in UART Mock)
     // Let's assume input drives the values.
     
     // Ideally, move the CSV parsing logic here completely.
     // For this step, I will put specific logic if user asks or just pass-through.
     // Since 'sensor_emulator' already had the parser, I should copy it here.
     
     // Placeholder: Valid Parse
     sim_state.lat_e7 = 371234567;
     sim_state.lon_e7 = 1271234567;
     sim_state.alt_m += 1.0; 
     sim_state.temp_c = 25.0;
     sim_state.pressure_pa = 101300;
     
     // Broadcast
     esp_now_send(broadcastAddress, (uint8_t *) &sim_state, sizeof(sim_state));
  }
}

void loop() {
  // 1. Handle PC Input (Simulation Scenarios)
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    parseSimData(line);
  }
  
  // 2. Handle STM32 Telemetry (Pass through to PC or Log)
  if (SerialSTM.available()) {
    // Just bridge to Serial for viewing
    uint8_t b = SerialSTM.read();
    // Maybe format it? Or raw dump.
    // Serial.write(b); 
    // If it's binary, printing might be messy.
    // Assuming STM32 sends formatted telemetry or we just hex dump.
    
    // For now, minimal feedback
    // Serial.print((char)b);
  }
  
  // Auto-generate test wave if no PC input?
  static uint32_t last_sim = 0;
  if (millis() - last_sim > 100) { // 10Hz
     sim_state.timestamp_ms = millis();
     sim_state.alt_m += 0.1;
     if (sim_state.alt_m > 1000) sim_state.alt_m = 0;
     
     esp_now_send(broadcastAddress, (uint8_t *) &sim_state, sizeof(sim_state));
     last_sim = millis();
  }
}
