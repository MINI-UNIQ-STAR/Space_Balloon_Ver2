#include <Arduino.h>
#include <Wire.h>

// --- MLX90393 Mock Configuration ---
#define I2C_SDA 6
#define I2C_SCL 7
#define MLX90393_ADDR 0x0C

volatile uint8_t last_cmd = 0;

// MLX90393 Protocol:
// 1. Master Write 1 byte (Command)
// 2. Master Read 1 byte (Status)
// OR
// 2. Master Read N bytes (Status + Data)

void onReceive(int len) {
  if (len > 0) {
    last_cmd = Wire.read();
    // Some commands have arguments, consume them if present
    while (Wire.available()) Wire.read(); 
  }
}

void onRequest() {
  // Always return Status Byte first
  // 0x00 = No Error, count = 0
  Wire.write(0x00); 

  // If it was a Measurement Read (RM), we might need to send more data
  // But Driver Init only sends EXIT(0x80), RESET(0xF0), CONF(0x60...)
  // All these expect just a Status Byte in return.
  
  // If Master asks for more bytes (e.g. reading data), fill with zeros
  // Wire.write() is valid to call multiple times? 
  // In ESP32 Slave mode, typically we just queue writes.
  // We'll just write a few more dummy bytes just in case.
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.write(0x00);
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL MLX90393 Mock (Address 0x0C)");

  if (!Wire.begin(MLX90393_ADDR, I2C_SDA, I2C_SCL, 100000)) {
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
