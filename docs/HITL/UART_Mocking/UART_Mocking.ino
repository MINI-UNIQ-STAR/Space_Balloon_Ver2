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

/* Helper: Send NMEA sentence with checksum */
void sendNMEASentence(String talker, String content) {
  String chk = calculateNMEAChecksum(content);
  GPS_SERIAL.print("$");
  GPS_SERIAL.print(talker);
  GPS_SERIAL.print(content);
  GPS_SERIAL.print("*");
  GPS_SERIAL.println(chk);
}

/* Helper: Send GSV (Satellites in View) Message */
void sendGSV(const char* talker, uint8_t total_sats) {
  // Simple GSV: 1 message showing total satellites
  // Format: GSV,num_msgs,msg_num,total_sats,sat_info...
  // Minimal implementation without individual satellite details
  String gsv = "GSV,1,1,";
  gsv += String(total_sats);
  gsv += ",,,,,,,,,,,,,,,,";  // Empty sat info fields (4 sats x 4 fields each)
  sendNMEASentence(String(talker), gsv);
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

  // Convert GPS coordinates to NMEA format
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

  // Format UTC time (HHMMSS.SS)
  char time_str[12];
  sprintf(time_str, "%02d%02d%02d.00", pkt->utc_hour, pkt->utc_min, pkt->utc_sec);

  // Format UTC date (DDMMYY)
  char date_str[8];
  int year_2digit = pkt->utc_year % 100;
  sprintf(date_str, "%02d%02d%02d", pkt->utc_day, pkt->utc_month, year_2digit);

  // 1. Send RMC (Recommended Minimum - includes time/date)
  String rmc = "RMC,";
  rmc += String(time_str) + ",A,";  // Time, Status=Active
  rmc += String(lat_str) + "," + lat_dir + ",";
  rmc += String(lon_str) + "," + lon_dir + ",";
  rmc += "0.0,0.0,";  // Speed over ground, Course over ground
  rmc += String(date_str) + ",,,A";  // Date, Magnetic variation, Mode
  sendNMEASentence("GP", rmc);

  // 2. Send GGA (Fix Data)
  String gga = "GGA,";
  gga += String(time_str) + ",";
  gga += String(lat_str) + "," + lat_dir + ",";
  gga += String(lon_str) + "," + lon_dir + ",";
  gga += String(pkt->fix_type) + "," + String(pkt->sats) + ",1.0,";
  gga += String(pkt->alt_m, 1) + ",M,0.0,M,,";
  sendNMEASentence("GP", gga);

  // 3. Send GSV Messages (Satellites in View) for each GNSS system
  if (pkt->sats_gps > 0) {
    sendGSV("GP", pkt->sats_gps);  // GPS
  }
  if (pkt->sats_glonass > 0) {
    sendGSV("GL", pkt->sats_glonass);  // GLONASS
  }
  if (pkt->sats_galileo > 0) {
    sendGSV("GA", pkt->sats_galileo);  // Galileo
  }
  if (pkt->sats_beidou > 0) {
    sendGSV("GB", pkt->sats_beidou);  // BeiDou
  }

  // 4. Generate PMS Frame
  sendPMSFrame(pkt->pm2_5, pkt->pm2_5, pkt->pm2_5);

  // Debug
  // Serial.printf("GPS: %s %s | Time: %s %s | Sats: GP=%d GL=%d GA=%d GB=%d\n",
  //               lat_str, lon_str, time_str, date_str,
  //               pkt->sats_gps, pkt->sats_glonass, pkt->sats_galileo, pkt->sats_beidou);
}

void loop() {
  // Parsing handled in callback
  delay(100);
}
