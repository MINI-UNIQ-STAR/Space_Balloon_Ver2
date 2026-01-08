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
// --- Timing Configuration ---
const uint32_t UPDATE_INTERVAL_MS = 20; // Fast sensor P+T
uint32_t last_update_ms = 0;

// Internal Buffer
float current_temp = 25.0;
float current_press = 101300.0;
uint32_t current_D1 = 0;
uint32_t current_D2 = 0;
uint8_t pending_conversion = 0; // 0=None, 1=D1(Press), 2=D2(Temp)

// Coefficients
const uint16_t C[7] = {0, 40127, 36924, 23317, 23282, 33464, 28312};

void updateRawValues() {
  // 1. Calculate D2 (Temp)
  // TEMP = 2000 + dT * C6 / 2^23
  // dT = (TEMP - 2000) * 2^23 / C6
  int32_t temp_c_100 = (int32_t)(current_temp * 100);
  int64_t dT = ((int64_t)temp_c_100 - 2000) * 8388608LL / C[6];
  current_D2 = (uint32_t)(dT + ((int64_t)C[5] << 8));
  
  // 2. Calculate D1 (Pressure)
  // OFF = C2 * 2^16 + (C4 * dT) / 2^7
  // SENS = C1 * 2^15 + (C3 * dT) / 2^8
  // P = (D1 * SENS / 2^21 - OFF) / 2^15
  // D1 = (P * 2^15 + OFF) * 2^21 / SENS
  
  int64_t OFF = ((int64_t)C[2] << 16) + (((int64_t)C[4] * dT) >> 7);
  int64_t SENS = ((int64_t)C[1] << 15) + (((int64_t)C[3] * dT) >> 8);
  
  int32_t P = (int32_t)current_press;
  
  // Inverse: D1 = (P * 32768 + OFF) * 2097152 / SENS
  // Note: Large numbers, need int64.
  // P*2^15 + OFF will be around (100000*32768 + 2e9) ~ 5e9 (fits in int64)
  // Then * 2^21 -> 1e16 (fits in int64, max is 9e18)
  
  int64_t numerator = ((int64_t)P * 32768LL + OFF) * 2097152LL;
  if(SENS != 0) current_D1 = (uint32_t)(numerator / SENS);
  else current_D1 = 0;
}

// --- ESP-NOW Callback ---
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  
  if (millis() - last_update_ms >= UPDATE_INTERVAL_MS) {
      current_temp = pkt->temp_c;
      current_press = pkt->pressure_pa;
      last_update_ms = millis();
  }
}

volatile uint8_t last_cmd = 0;

void onReceive(int len) {
  if (len > 0) {
    last_cmd = Wire.read();
    
    // Check conversion commands
    // D1 (Pressure): 0x40, 0x42, 0x44, 0x46, 0x48 (OSR)
    // D2 (Temp): 0x50, 0x52, 0x54, 0x56, 0x58 (OSR)
    if ((last_cmd & 0xF0) == 0x40) {
        pending_conversion = 1; // D1
    } else if ((last_cmd & 0xF0) == 0x50) {
        pending_conversion = 2; // D2
    }
    
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
         C[1],  // C1
         C[2],  // C2
         C[3],  // C3
         C[4],  // C4
         C[5],  // C5
         C[6],  // C6
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
    uint32_t adc_val = 0;
    if (pending_conversion == 1) adc_val = current_D1;
    else if (pending_conversion == 2) adc_val = current_D2;
    else adc_val = 0; // Should not happen or Reset?
    
    Wire.write((adc_val >> 16) & 0xFF);
    Wire.write((adc_val >> 8) & 0xFF);
    Wire.write(adc_val & 0xFF);
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
  updateRawValues();
  delay(10);
}
