#include <Arduino.h>
#include <Wire.h>

// --- I2C Mock Configuration ---
#define I2C_SDA 6
#define I2C_SCL 7
#define I2C_LSM6DSV16X_ADDR 0x6B
#define LSM6DSV16X_ID 0x70  // Expected WHO_AM_I value

volatile uint8_t current_reg = 0;

// I2C Receive Handler (STM32 writes Register Address)
void onReceive(int len) {
  if (len > 0) {
    current_reg = Wire.read();
    while (Wire.available()) Wire.read(); // Consume extra if any
  }
}

// I2C Request Handler (STM32 reads Data)
void onRequest() {
  uint8_t val = 0;
  
  if (current_reg == 0x0F) { // WHO_AM_I
    val = LSM6DSV16X_ID; // 0x70
  } 
  else {
    // Return dummy data for other registers (e.g. Accel/Gyro data)
    val = 0x00;
  }
  
  Wire.write(val);
}

void setup() {
  Serial.begin(115200);
  
  Serial.println("HITL I2C Mock Device (LSM6DSV16X Emulation)");
  Serial.println("Note: LoRa functionality disabled for I2C-only HITL test.");

  // 1. Setup I2C Slave (Mocking LSM6DSV16X)
  // ESP32 Wire.begin(addr, sda, scl, freq)
  if (!Wire.begin(I2C_LSM6DSV16X_ADDR, I2C_SDA, I2C_SCL, 100000)) {
    Serial.println("I2C Slave Init Failed!");
  } else {
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Serial.println("I2C Slave Active (Addr 0x6B) on Pins 6(SDA), 7(SCL)");
  }
}

void loop() {
  // I2C Task is Interrupt-Driven
  delay(10);
}
