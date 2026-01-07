/*
 * HITL Ground Station Firmware for ESP32-C3
 * 
 * Hardware Roles:
 * - LoRa Receiver: Receives telemetry from Flight Computer.
 * - USB Serial: Displays data and FDIR status to PC.
 * 
 * Pinout (Standard ESP32-C3 LoRa board, e.g., Xiao ESP32C3 with LoRa shield or similar):
 * - Adjust pins as per your specific hardware! 
 * - Defaulting to generic SPI pins for now.
 */

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// --- Pin Definitions (Adjust for your ESP32-C3 board) ---
// Example pins for a custom carrier or standard dev board
#define SCK     8
#define MISO    9
#define MOSI    10
#define SS      20
#define RST     21
#define DIO0    2   // Check your board's IRQ pin

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Ground Station (ESP32-C3) Starting...");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    String packet = "";
    while (LoRa.available()) {
      packet += (char)LoRa.read();
    }
    
    int rssi = LoRa.packetRssi();
    
    // Parse "TLM,<Flags>,<Lat>,<Lon>..."
    Serial.print("[RX] RSSI: " + String(rssi) + " | Data: " + packet);
    
    // Check FDIR Flags
    if (packet.startsWith("TLM")) {
      int firstComma = packet.indexOf(',');
      int secondComma = packet.indexOf(',', firstComma + 1);
      if (firstComma > 0 && secondComma > 0) {
        int flags = packet.substring(firstComma + 1, secondComma).toInt();
        
        Serial.print(" | Status: ");
        if (flags == 0) Serial.print("OK");
        if (flags & (1<<1)) Serial.print(" GPS_WARN");
        if (flags & (1<<2)) Serial.print(" BARO_WARN");
      }
    }
    Serial.println();
  }
}
