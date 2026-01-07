/*
 * HITL Sensor Emulator Firmware (ESP-NOW Satellite Node)
 * 
 * INPUT: HitlStatePacket via ESP-NOW from Main Control
 * OUTPUT 1 (GPS_SERIAL): Standard NMEA ($GPGGA) sentences
 * OUTPUT 2 (AUX_SERIAL): Standard PMS3003 Binary Frames
 */

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

#define GPS_TX_PIN 17
#define GPS_RX_PIN 16
#define AUX_TX_PIN 19
#define AUX_RX_PIN 18

#define GPS_SERIAL Serial1
#define AUX_SERIAL Serial2

void setup() {
  Serial.begin(115200);
  
  // GPS: Standard NMEA 9600
  GPS_SERIAL.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  // PMS: Standard 9600
  AUX_SERIAL.begin(9600, SERIAL_8N1, AUX_RX_PIN, AUX_TX_PIN);
  
  Serial.println("HITL UART Emulator (ESP-NOW Mode)");

  // Init Wifi
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register Callback
  esp_now_register_recv_cb(OnDataRecv);
}

/* Helper: Checksum for NMEA */
String calculateNMEAChecksum(String content) {
  int sum = 0;
  for (int i = 0; i < content.length(); i++) {
    sum ^= content.charAt(i);
  }
  String hex = String(sum, HEX);
  hex.toUpperCase();
  if (hex.length() < 2) hex = "0" + hex;
  return hex;
}

/* Helper: Send PMS3003 Data */
void sendPMSFrame(uint16_t pm1, uint16_t pm25, uint16_t pm10) {
  uint8_t buffer[32];
  uint16_t frameLen = 28; // Standard length
  
  buffer[0] = 0x42;
  buffer[1] = 0x4D;
  buffer[2] = (frameLen >> 8) & 0xFF;
  buffer[3] = frameLen & 0xFF;
  
  // Payload (Standard + Atmospheric)
  // Standard (1.0, 2.5, 10.0) -> Bytes 4-9
  buffer[4] = (pm1 >> 8) & 0xFF;  buffer[5] = pm1 & 0xFF;
  buffer[6] = (pm25 >> 8) & 0xFF; buffer[7] = pm25 & 0xFF;
  buffer[8] = (pm10 >> 8) & 0xFF; buffer[9] = pm10 & 0xFF;
  
  // Atmospheric (1.0, 2.5, 10.0) -> Bytes 10-15 (Same for mock)
  buffer[10] = (pm1 >> 8) & 0xFF; buffer[11] = pm1 & 0xFF;
  buffer[12] = (pm25 >> 8) & 0xFF; buffer[13] = pm25 & 0xFF;
  buffer[14] = (pm10 >> 8) & 0xFF; buffer[15] = pm10 & 0xFF;
  
  // Remaining bytes (counts, version, error) - Zero out
  for(int i=16; i<30; i++) buffer[i] = 0;
  
  // Checksum (Sum of bytes 0-29)
  uint16_t checksum = 0;
  for(int i=0; i<30; i++) checksum += buffer[i];
  
  buffer[30] = (checksum >> 8) & 0xFF;
  buffer[31] = checksum & 0xFF;
  
  AUX_SERIAL.write(buffer, 32);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  
  // 1. Generate NMEA $GPGGA using pkt data
  // data: lat_e7, lon_e7, alt_m
  
  double lat_deg = pkt->lat_e7 / 10000000.0;
  int lat_d = (int)abs(lat_deg);
  double lat_m = (abs(lat_deg) - lat_d) * 60.0;
  char lat_str[15];
  sprintf(lat_str, "%02d%07.4f", lat_d, lat_m);
  char lat_dir = (pkt->lat_e7 >= 0) ? 'N' : 'S';
  
  double lon_deg = pkt->lon_e7 / 10000000.0;
  int lon_d = (int)abs(lon_deg);
  double lon_m = (abs(lon_deg) - lon_d) * 60.0;
  char lon_str[15];
  sprintf(lon_str, "%03d%07.4f", lon_d, lon_m);
  char lon_dir = (pkt->lon_e7 >= 0) ? 'E' : 'W';
  
  // Time from timestamp? Mocking 120000.00
  String nmea = "GPGGA,120000.00,";
  nmea += String(lat_str) + "," + lat_dir + ",";
  nmea += String(lon_str) + "," + lon_dir + ",";
  nmea += String(pkt->fix_type) + "," + String(pkt->sats) + ",1.0,";
  nmea += String(pkt->alt_m, 1) + ",M,0.0,M,,";
  
  String chk = calculateNMEAChecksum(nmea);
  GPS_SERIAL.print("$");
  GPS_SERIAL.print(nmea);
  GPS_SERIAL.print("*");
  GPS_SERIAL.println(chk);
  
  // 2. Generate PMS Frame
  sendPMSFrame(pkt->pm2_5, pkt->pm2_5, pkt->pm2_5); // Using PM2.5 for all for simplicity if others not in struct
  
  // Debug
  // Serial.printf("Updated GPS: %f, %f, %f\n", lat_deg, lon_deg, pkt->alt_m);
}

void loop() {
  // Parsing handled in callback
  delay(100);
}
