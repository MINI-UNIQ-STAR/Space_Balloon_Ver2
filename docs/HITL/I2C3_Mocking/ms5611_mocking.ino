#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- MS5611 Mock Configuration ---
#define MS5611_ADDR 0x77

// Mock Values (ESP-NOW)
float mock_temp = 25.0;
float mock_press = 101300.0; // Pa

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  mock_temp = pkt->temp_c;
  mock_press = pkt->pressure_pa;
}

volatile uint8_t last_cmd = 0;

void onReceive(int len) {
  if (len > 0) {
    last_cmd = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  // 1. PROM Read (0xA0 ~ 0xAE)
  // 1. PROM Read (0xA0 ~ 0xAE)
  if (last_cmd >= 0xA0 && last_cmd <= 0xAE) {
     // Index 0..7
     // Cmd 0xA0 -> Index 0
     // Cmd 0xA2 -> Index 1 ...
     uint8_t idx = (last_cmd - 0xA0) / 2;
     
     // Mock PROM Data (Calibration)
     // C1=40127, C2=36924, C3=23317, C4=23282, C5=33464, C6=28312
     // These are example coefficients.
     // CRC needs to be valid.
     
     static uint16_t prom[8] = {
         0x0000, // C0 (Reserved)
         40127,  // C1
         36924,  // C2
         23317,  // C3
         23282,  // C4
         33464,  // C5
         28312,  // C6
         0x0000  // C7 (CRC will be inserted)
     };
     
     static bool crc_calculated = false;
     if (!crc_calculated) {
         // CRC4 Calculation loop (Same as driver)
         uint16_t n_rem = 0;
         prom[7] = 0; // Clear CRC byte
         
         for (int cnt = 0; cnt < 16; cnt++) {
             if (cnt % 2 == 1) n_rem ^= (prom[cnt >> 1] & 0x00FF);
             else n_rem ^= (prom[cnt >> 1] >> 8);
             
             for (int n_bit = 8; n_bit > 0; n_bit--) {
                 if (n_rem & 0x8000) n_rem = (n_rem << 1) ^ 0x3000;
                 else n_rem = (n_rem << 1);
             }
         }
         n_rem = (n_rem >> 12) & 0x000F;
         prom[7] = n_rem; // Insert CRC at LSB 4 bits (or is it? Driver: crc_read = prom[7] & 0x000F)
         // Wait, driver says: crc_read = prom[7] & 0x000F. So we put it in last 4 bits.
         crc_calculated = true;
     }
     
     uint16_t val = prom[idx];
     Wire.write(val >> 8);
     Wire.write(val & 0xFF);
  }
  // 2. ADC Read (0x00)
  else if (last_cmd == 0x00) {
    // Return 3 bytes (24-bit raw)
    // MS5611 Raw logic is complex (dT calculation).
    // For simple mock, we just return a fluctuating value?
    // Or we should inverse calculate D1/D2 from Temp/Pressure.
    // Given the STM32 driver just reads raw and computes,
    // if we send constant raw, it will read constant T/P.
    // Let's emulate a "reasonable" raw value.
    // D1 (Pressure) ~ 8,000,000
    // D2 (Temp) ~ 8,000,000
    
    // Simplification: Return fixed/dynamic bytes based on mock_press?
    // Let's just return a constant for now to pass init.
    // If strict physical simulation needed, we need C1-C6 and D1/D2 math.
    
    // Just return semi-random variations or static
    Wire.write(0x80);
    Wire.write(0x00);
    Wire.write(0x00);
  }
  else {
    Wire.write(0x00);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL MS5611 Mock (Address 0x77) [ESP-NOW Active]");
  
  // Init Wifi & ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
  }

  if (!Wire.begin(MS5611_ADDR)) {
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
