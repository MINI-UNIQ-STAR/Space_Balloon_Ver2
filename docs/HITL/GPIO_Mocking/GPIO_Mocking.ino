#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Configuration ---
// Adjust pins to match your ESP32-to-STM32 wiring
// ESP32 Input Pin -> STM32 Output Pin

// 1. OneWire (DS18B20 Mock)
#define ONE_WIRE_PIN 4  // Connect to STM32 PB15

// 2. Heater Monitors (PWM)
#define HEATER_BAT_PIN 18 // Connect to STM32 PA6 (TIM3_CH1) / PC6? Check Actuators.c: PA6 is Heater 1
#define HEATER_BD_PIN  19 // Connect to STM32 PC6 (TIM8_CH1)

// 3. Reset Monitors (Active Low)
#define RST_XA1110_PIN 25 // PA9 ? Check main.h
#define RST_PMS_PIN    26 // PB10
#define RST_GDK_PIN    27 // PB2
#define RST_SEN_PIN    14 // PB1
#define RST_CM_PIN     12 // PB0
#define RST_MCP_PIN    13 // PA4
#define RST_MS_PIN     32 // PA5
#define RST_SHT_PIN    33 // PB11
#define RST_LSM_PIN    34 // PB13
#define RST_MLX_PIN    35 // PB14

// OneWire Global Mock Temperatures (Updated via ESP-NOW)
float mock_temp_1 = 25.0; // Device 1 (Battery)
float mock_temp_2 = 30.0; // Device 2 (Board)

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  
  // Update mock environment
  // DS18B20 usually measures Battery/Board temp.
  // In simulation, we might map 'temp_c' to Board Temp
  // and maybe 'temp_c + 5' to Battery Temp?
  mock_temp_1 = pkt->temp_c;      // Battery
  mock_temp_2 = pkt->temp_c + 2.0; // Board (offset for variety)
}

// --- OneWire Mock Logic (Enhanced for 2 Devices) ---
// ROM Codes (Family 0x28 for DS18B20)
uint8_t ROM1[8] = {0x28, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}; // Need valid CRC
uint8_t ROM2[8] = {0x28, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x4E}; // Random

// Helper to calc CRC8 for ROM
uint8_t calcCRC8(uint8_t *addr, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t inbyte = addr[i];
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

void initROMs() {
  ROM1[7] = calcCRC8(ROM1, 7);
  ROM2[7] = calcCRC8(ROM2, 7);
}

uint8_t selected_device = 0; // 0=None, 1=ROM1, 2=ROM2, 3=Both(SkipROM)

void sendBit(uint8_t bit) {
  // Wait for Master to pull Low
  while(digitalRead(ONE_WIRE_PIN) == HIGH);
  
  if (bit == 1) {
    // Release immediately (Master pulls low for <15us)
    // We just wait for it to go high
  } else {
    // Drive Low for 30us (Mock Slave dominant) 
    pinMode(ONE_WIRE_PIN, OUTPUT);
    digitalWrite(ONE_WIRE_PIN, LOW);
    delayMicroseconds(30); 
    digitalWrite(ONE_WIRE_PIN, HIGH);
    pinMode(ONE_WIRE_PIN, INPUT);
  }
  // Wait for Master to release
  while(digitalRead(ONE_WIRE_PIN) == LOW);
}

uint8_t readBit() {
  // Wait for start
  while(digitalRead(ONE_WIRE_PIN) == HIGH);
  delayMicroseconds(10); // Sample point
  uint8_t b = digitalRead(ONE_WIRE_PIN);
  while(digitalRead(ONE_WIRE_PIN) == LOW);
  return b;
}

void writeByte(uint8_t val) {
  for (int i=0; i<8; i++) {
    sendBit(val & 1);
    val >>= 1;
  }
}

uint8_t readByte() {
  uint8_t val = 0;
  for (int i=0; i<8; i++) {
    if (readBit()) val |= (1 << i);
  }
  return val;
}

// Search ROM State Support
void runSearchRom() {
  // 2 Devices participating
  bool dev1_active = true;
  bool dev2_active = true;
  
  for (int bit_idx = 0; bit_idx < 64; bit_idx++) {
    int byte_idx = bit_idx / 8;
    int bit_offset = bit_idx % 8;
    
    // 1. Read Current Bits from active devices
    uint8_t b1 = (ROM1[byte_idx] >> bit_offset) & 1;
    uint8_t b2 = (ROM2[byte_idx] >> bit_offset) & 1;
    
    uint8_t bit_true = 1;
    if (dev1_active && b1 == 0) bit_true = 0;
    if (dev2_active && b2 == 0) bit_true = 0;
    
    sendBit(bit_true);
    
    // 2. Send Complement Bits
    uint8_t bit_comp = 1;
    if (dev1_active && b1 == 1) bit_comp = 0; // Compl is 0
    if (dev2_active && b2 == 1) bit_comp = 0;
    
    sendBit(bit_comp);
    
    // 3. Master sends direction
    uint8_t doc_dir = readBit();
    
    // 4. Update Active Status
    if (dev1_active) {
      if (b1 != doc_dir) dev1_active = false;
    }
    if (dev2_active) {
      if (b2 != doc_dir) dev2_active = false;
    }
    
    if (!dev1_active && !dev2_active) return; // All lost match
  }
  
  // End of Search: Master found one logic path
  if (dev1_active) selected_device = 1;
  else if (dev2_active) selected_device = 2;
  else selected_device = 0;
}

void runOneWireMock() {
  if (digitalRead(ONE_WIRE_PIN) == LOW) {
    uint32_t start = micros();
    while (digitalRead(ONE_WIRE_PIN) == LOW);
    if (micros() - start > 400) {
      // RESET detected
      delayMicroseconds(30);
      pinMode(ONE_WIRE_PIN, OUTPUT);
      digitalWrite(ONE_WIRE_PIN, LOW); // Presence
      delayMicroseconds(120);
      digitalWrite(ONE_WIRE_PIN, HIGH);
      pinMode(ONE_WIRE_PIN, INPUT);
      
      // Wait for Command
      uint8_t cmd = readByte();
      
      if (cmd == 0xCC) { // SKIP ROM
        selected_device = 3; // Both (Warning: Read collision if 2 devices)
      }
      else if (cmd == 0xF0) { // SEARCH ROM
        runSearchRom();
        return; 
      }
      else if (cmd == 0x55) { // MATCH ROM
        // Read 8 bytes (ROM)
        uint8_t target[8];
        for(int i=0; i<8; i++) target[i] = readByte();
        
        // Check Match
        bool match1 = true;
        bool match2 = true;
        for(int i=0; i<8; i++) {
            if(target[i] != ROM1[i]) match1 = false;
            if(target[i] != ROM2[i]) match2 = false;
        }
        
        if (match1) selected_device = 1;
        else if (match2) selected_device = 2;
        else selected_device = 0;
      }
      else {
          return; // Unknown
      }
      
      // Function Command (if device selected)
      if (selected_device != 0) {
        uint8_t func = readByte();
        if (func == 0x44) { // CONVERT T
            // Start conversion (Mock: do nothing, data is ready)
        }
        else if (func == 0xBE) { // READ SCRATCHPAD
            float t = (selected_device == 1) ? mock_temp_1 : mock_temp_2;
            int16_t raw = (int16_t)(t * 16.0);
            
            writeByte(raw & 0xFF);
            writeByte((raw >> 8) & 0xFF);
            for(int k=0; k<7; k++) writeByte(0);
        }
      }
    }
  }
}

// --- Heater Monitoring ---
void checkHeaters() {
  static uint32_t last_print = 0;
  if (millis() - last_print > 1000) {
    long duration1 = pulseIn(HEATER_BAT_PIN, HIGH, 20000); // Timeout 20ms
    long duration2 = pulseIn(HEATER_BD_PIN, HIGH, 20000);
    
    if (duration1 > 0 || duration2 > 0) {
      Serial.printf("Heaters: Bat PWM ~%lu us | Board PWM ~%lu us\n", duration1, duration2);
    }
    last_print = millis();
  }
}

// --- Reset Monitoring ---
// Polling Resets (Fast)
void checkResets() {
  // Simple state check
  static uint32_t last_rst[10] = {0};
  uint8_t pins[] = {RST_XA1110_PIN, RST_PMS_PIN, RST_GDK_PIN, RST_LSM_PIN, RST_MLX_PIN, RST_MS_PIN, RST_SHT_PIN, RST_CM_PIN, RST_MCP_PIN, RST_SEN_PIN};
  const char* names[] = {"GPS", "PMS", "GDK", "LSM", "MLX", "MS5611", "SHT", "CM1107", "MCP", "SEN"};
  
  for(int i=0; i<10; i++) {
    if (digitalRead(pins[i]) == LOW) {
      if (millis() - last_rst[i] > 1000) { // Throttle logs
        Serial.printf("RESET DETECTED: %s\n", names[i]);
        last_rst[i] = millis();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL GPIO Mocking (OneWire + Heaters + Resets)");
  
  // Init ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("ESP-NOW Recv Active");
  
  initROMs(); // Calculate CRCs for Mock ROMs

  pinMode(ONE_WIRE_PIN, INPUT); // External Pull-up required on bus!
  
  pinMode(HEATER_BAT_PIN, INPUT);
  pinMode(HEATER_BD_PIN, INPUT);
  
  uint8_t rst_pins[] = {RST_XA1110_PIN, RST_PMS_PIN, RST_GDK_PIN, RST_LSM_PIN, RST_MLX_PIN, RST_MS_PIN, RST_SHT_PIN, RST_CM_PIN, RST_MCP_PIN, RST_SEN_PIN};
  for(int i=0; i<10; i++) pinMode(rst_pins[i], INPUT_PULLUP);
}

void loop() {
  runOneWireMock(); // Must run frequently/blocking when needed
  checkHeaters();   // Periodic
  checkResets();    // Fast poll
}
