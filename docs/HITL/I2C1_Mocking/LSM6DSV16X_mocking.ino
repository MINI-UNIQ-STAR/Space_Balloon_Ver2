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
uint8_t registers[256];

void updateSensorRegisters() {
  // Convert float to int16 based on default sensitivity (2g, 2000dps for simplicity)
  // Real implementation would check CTRL1/CTRL2
  // Accel: 2g range -> 0.061 mg/LSB -> 1g = 16393 LSB
  // Gyro: 2000dps -> 70 mdps/LSB -> 1 dps = 14.28 LSB
  
  int16_t ax = (int16_t)(mock_accel[0] * 16393.0f);
  int16_t ay = (int16_t)(mock_accel[1] * 16393.0f);
  int16_t az = (int16_t)(mock_accel[2] * 16393.0f);
  
  int16_t gx = (int16_t)(mock_gyro[0] * 14.28f);
  int16_t gy = (int16_t)(mock_gyro[1] * 14.28f);
  int16_t gz = (int16_t)(mock_gyro[2] * 14.28f);
  
  // Gyro: 0x22 - 0x27
  registers[0x22] = gx & 0xFF; registers[0x23] = (gx >> 8) & 0xFF;
  registers[0x24] = gy & 0xFF; registers[0x25] = (gy >> 8) & 0xFF;
  registers[0x26] = gz & 0xFF; registers[0x27] = (gz >> 8) & 0xFF;
  
  // Accel: 0x28 - 0x2D
  registers[0x28] = ax & 0xFF; registers[0x29] = (ax >> 8) & 0xFF;
  registers[0x2A] = ay & 0xFF; registers[0x2B] = (ay >> 8) & 0xFF;
  registers[0x2C] = az & 0xFF; registers[0x2D] = (az >> 8) & 0xFF;
}

void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    len--;
    
    // If writing data to register
    while (len > 0 && Wire.available()) {
      registers[current_reg++] = Wire.read();
      len--;
    }
  }
}

void onRequest() {
  // Return value and auto-increment
  Wire.write(registers[current_reg]);
  // Note: Some drivers rely on repeated start without stop, so current_reg should persist?
  // Standard I2C: Ack -> Master Reads -> Slave Incr.
  // Wire library behavior: onRequest called once per requested batch? No, onRequest called per request transaction.
  // But usage suggests just returning the byte.
  
  // Auto-increment for burst reads
  // The 'onRequest' is called once. If master requests M bytes, we write M bytes?
  // Wire.write buffer size?
  // ESP32 Wire Slave behavior: write() queues data. 
  // We should write as many as possible or rely on repeated calls?
  // Actually, standard Wire slave onRequest expects you to write everything the master asked for?
  // No, onRequest doesn't know how many bytes master wants.
  // ESP32 I2C Slave is tricky.
  // Reverting to simple single-byte or explicitly buffered if library supports it.
  // Standard Arduino Wire: One write() call.
  // Let's assume the driver reads byte-by-byte or block.
  // If block read, we might need a loop if onRequest allows it.
  // For now: Just write one byte. If Master NACKs, fine.
  // Actually, if we want to support burst read of registers, we might need to write more?
  // ESP32 I2C Slave usually writes until buffer full or NACK.
  
  // Safe implementation:
  // Since we don't know length, just Write the current one.
  // And maybe increment logic isn't easily possible here without knowing if it was ACKed.
  // But for WHO_AM_I check, single byte is fine.
  // For Data read (6 bytes), if driver does Read(6), we need to send 6 bytes.
  
  // Hack: Setup output buffer for next few registers?
  // Wire.write(&registers[current_reg], 32); // Fill buffer with subsequent registers
  // This is a common strategy for I2C slaves on Arduino.
  size_t max_len = 256 - current_reg;
  if(max_len > 32) max_len = 32;
  Wire.write(&registers[current_reg], max_len);
  
  // We can't update current_reg here accurately because we don't know how many were valued.
  // But typically the master sets the address then reads.
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
  
  // Init Registers
  memset(registers, 0, 256);
  registers[0x0F] = LSM6DSV16X_ID; // WHO_AM_I

  if (!Wire.begin(I2C_LSM6DSV16X_ADDR, I2C_SDA, I2C_SCL, 100000)) {
    Serial.println("I2C Slave Init Failed!");
  } else {
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Serial.println("I2C Slave Active (Addr 0x6B)");
  }
}

void loop() {
  updateSensorRegisters();
  delay(10);
}
