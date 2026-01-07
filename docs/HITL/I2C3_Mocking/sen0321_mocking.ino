#include <Arduino.h>
#include <Wire.h>

// --- SEN0321 (Ozone) Mock Configuration ---
// I2C Address from sen0321_driver.h (SEN0321_I2C_ADDR_0 = 0x70)
// Or SEN0321_I2C_ADDR_0 can be 0x70, 0x71, 0x72, 0x73 depending on DIP switch.
// Driver uses SEN0321_I2C_ADDR_0 (0x70) by default.
#define I2C_ADDR_SEN0321 0x70

#define I2C_SDA 21  // Adjust as needed for generic ESP32 or specific board
#define I2C_SCL 22

// Registers
#define REG_MODE 0x03
#define REG_AUTO_DATA_H 0x09
#define REG_AUTO_DATA_L 0x0A

volatile uint8_t current_reg = 0;

void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    while (Wire.available()) Wire.read();
  }
}

void onRequest() {
  // If Master reads register:
  // Driver typically writes Reg, then Reads 2 bytes (Data H, Data L)
  
  if (current_reg == REG_AUTO_DATA_H) {
    // Return Ozone PPB (e.g. 20 ppb)
    // 20 = 0x0014
    Wire.write(0x00); // High Byte
    Wire.write(0x14); // Low Byte
  }
  else if (current_reg == REG_MODE) {
    // Driver writes to this, doesn't usually read back.
    // If read, return current mode (0 = Auto)
    Wire.write(0x00);
  }
  else {
    Wire.write(0x00); 
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL SEN0321 (Ozone) Mock (Address 0x70)");

  // Use default I2C pins or defined ones
  // For ESP32 w/ Arduino, Wire.begin(ADDR) uses default pins (21, 22 on generic ESP32)
  // If specific pins needed: Wire.begin(I2C_ADDR_SEN0321, sda, scl, freq)
  if (!Wire.begin(I2C_ADDR_SEN0321)) {
    Serial.println("I2C Init Failed");
  } else {
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Serial.println("I2C Slave Active (0x70)");
  }
}

void loop() {
  delay(10);
}
