#include <Arduino.h>
#include <Wire.h>

// --- GDK101 Mock Configuration ---
#define I2C_SDA 6
#define I2C_SCL 7
#define GDK101_ADDR 0x18

// Registers (minimal set for Mock)
#define REG_STATUS 0x00 // Assumed or Generic

volatile uint8_t current_reg = 0;

// GDK101: 
// 1. Master Write 1 byte (Register Address)
// 2. Master Read N bytes (Data)

void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  // GDK101 Driver typically reads 2 bytes (Value LSB, Value MSB or similar)
  // For Status: [Status, Vibration]
  // We return 0 (OK) for both.
  
  Wire.write(0x00);
  Wire.write(0x00);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL GDK101 Mock (Address 0x18)");

  if (!Wire.begin(GDK101_ADDR, I2C_SDA, I2C_SCL, 100000)) {
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
