#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Configuration: Node A (LoRa32 #1) ---
// I2C Port 0 (Wire) -> LSM6DSV16X
#define I2C0_SDA 21
#define I2C0_SCL 22
#define LSM_ADDR 0x6B
#define LSM_ID   0x70

// I2C Port 1 (Wire1) -> MLX90393
#define I2C1_SDA 13
#define I2C1_SCL 12
#define MLX_ADDR 0x0C

#define LED_PIN 2

// Mock Physic State
volatile float acc[3] = {0,0,9.8};
volatile float gyr[3] = {0,0,0};
volatile float mag[3] = {30,0,40}; // uT

// --- ESP-NOW Handler ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  
  // Update Physics
  acc[0] = pkt->accel[0]; acc[1] = pkt->accel[1]; acc[2] = pkt->accel[2];
  gyr[0] = pkt->gyro[0];  gyr[1] = pkt->gyro[1];  gyr[2] = pkt->gyro[2];
  mag[0] = pkt->mag[0];   mag[1] = pkt->mag[1];   mag[2] = pkt->mag[2];

  // LED Heartbeat
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// --- Wire 0 Logic (LSM6DSV) ---
volatile uint8_t lsm_reg = 0;
void onReceive0(int len) {
  if (len > 0) {
    lsm_reg = Wire.read();
    while(Wire.available()) Wire.read();
  }
}
void onRequest0() {
  uint8_t val = 0;
  // Minimal Register Map for LSM6DSV16X
  if (lsm_reg == 0x0F) val = LSM_ID; // WHO_AM_I
  
  // Real data registers (OUTX_L_G etc) ideally should be implemented.
  // For basic checks, we return 0. (Proper implementation is huge).
  // If user needs full IMU data, we need map logic here.
  // Assuming 'Simple Init Check' first.
  
  Wire.write(val);
}

// --- Wire 1 Logic (MLX90393) ---
// MLX protocol is complex (Status byte + Data).
// Ideally we just ACK or return fixed status.
volatile uint8_t mlx_reg = 0; 
void onReceive1(int len) {
  while(Wire1.available()) Wire1.read(); // Consume
}
void onRequest1() {
  // MLX Status Byte (Simple)
  Wire1.write(0x00);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL Node A: I2C1 Dual Mock (LSM+MLX)");

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(OnDataRecv);
  } else {
    Serial.println("ESP-NOW Error");
  }

  // Init Dual I2C
  // Port 0: LSM
  if (!Wire.begin(LSM_ADDR, I2C0_SDA, I2C0_SCL, 400000)) {
     Serial.println("Wire(0) Init Failed");
  } else {
     Wire.onReceive(onReceive0);
     Wire.onRequest(onRequest0);
     Serial.println("Wire(0) Active: LSM6DSV (0x6B)");
  }

  // Port 1: MLX
  if (!Wire1.begin(MLX_ADDR, I2C1_SDA, I2C1_SCL, 100000)) {
     Serial.println("Wire(1) Init Failed");
  } else {
     Wire1.onReceive(onReceive1);
     Wire1.onRequest(onRequest1);
     Serial.println("Wire(1) Active: MLX90393 (0x0C)");
  }
  
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  delay(100);
}
