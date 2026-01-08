#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Configuration: Node D (LoRa32 #3 - Spare) ---
// I2C Port 0 (Wire) -> CM1107N
#define I2C0_SDA 21
#define I2C0_SCL 22
#define CM_ADDR  0x31

// I2C Port 1 (Wire1) -> MCP9600
// Note: Check available pins on your TTGO LoRa32 v2.1
#define I2C1_SDA 32 // Example
#define I2C1_SCL 33 // Example (If available)
#define MCP_ADDR 0x60

// UART 1 (GPS - XA1110)
// STM32 UART3_RX (PC11) <-> ESP32 TX
// STM32 UART3_TX (PC10) <-> ESP32 RX
#define GPS_TX_PIN 17
#define GPS_RX_PIN 16
#define GPS_SERIAL Serial1

// UART 2 (PMS3003)
#define PMS_TX_PIN 4  // Example
#define PMS_RX_PIN 15 // Example
#define PMS_SERIAL Serial2

#define LED_PIN 2

// Mock State
float co2 = 400;
float mcp_temp = 25.0;
int32_t lat_e7 = 350000000, lon_e7 = 1270000000;
float alt = 100.0;
uint8_t fix_type = 1, sats = 8;
uint8_t sats_gps = 5, sats_glonass = 2, sats_galileo = 1, sats_beidou = 0;
uint8_t utc_hour = 12, utc_min = 0, utc_sec = 0;
uint8_t utc_day = 1, utc_month = 1;
uint16_t utc_year = 2026;
float pm25 = 10;

// --- ESP-NOW Handler ---
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(HitlStatePacket)) return;
  HitlStatePacket *pkt = (HitlStatePacket*)incomingData;
  co2 = pkt->co2;
  mcp_temp = pkt->temp_c; // using ambient for TC
  lat_e7 = pkt->lat_e7;
  lon_e7 = pkt->lon_e7;
  alt = pkt->alt_m;
  fix_type = pkt->fix_type;
  sats = pkt->sats;
  sats_gps = pkt->sats_gps;
  sats_glonass = pkt->sats_glonass;
  sats_galileo = pkt->sats_galileo;
  sats_beidou = pkt->sats_beidou;
  utc_hour = pkt->utc_hour;
  utc_min = pkt->utc_min;
  utc_sec = pkt->utc_sec;
  utc_day = pkt->utc_day;
  utc_month = pkt->utc_month;
  utc_year = pkt->utc_year;
  pm25 = pkt->pm2_5;

  // LED Heartbeat
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

// --- Wire 0 (CM1107N) ---
void onRequest0() {
  // CM1107N Protocol (Simplified)
  // Send 8 bytes response
  static uint8_t resp[8] = {0x16,0x05,0x01,0x01,0x90,0,0,0}; // 400ppm
  int val = (int)co2;
  resp[3] = (val >> 8) & 0xFF;
  resp[4] = val & 0xFF;
  Wire.write(resp, 8);
}
void onReceive0(int len) {
  while(Wire.available()) Wire.read();
}

// --- Wire 1 (MCP9600) ---
volatile uint8_t mcp_reg = 0;
void onReceive1(int len) {
  if(len>0) {
    mcp_reg = Wire1.read();
    while(Wire1.available()) Wire1.read();
  }
}
void onRequest1() {
  // MCP9600 Read (Hot Junction 0x00, Delta 0x01, Cold 0x02)
  // Return fixed temperature
  int16_t t_raw = (int16_t)(mcp_temp * 16.0);
  Wire1.write((t_raw >> 8) & 0xFF);
  Wire1.write(t_raw & 0xFF);
}

// --- UART Logic ---
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

void sendNMEASentence(String talker, String content) {
  String chk = calculateNMEAChecksum(content);
  GPS_SERIAL.print("$");
  GPS_SERIAL.print(talker);
  GPS_SERIAL.print(content);
  GPS_SERIAL.print("*");
  GPS_SERIAL.println(chk);
}

void sendGSV(const char* talker, uint8_t total_sats) {
  String gsv = "GSV,1,1,";
  gsv += String(total_sats);
  gsv += ",,,,,,,,,,,,,,,,";
  sendNMEASentence(String(talker), gsv);
}

void runGPSMock() {
  static uint32_t last_gps = 0;
  if(millis() - last_gps > 1000) {
    // Convert coordinates to NMEA format
    double lat_deg = lat_e7 / 10000000.0;
    int lat_d = (int)abs(lat_deg);
    double lat_m = (abs(lat_deg) - lat_d) * 60.0;
    char lat_str[15];
    sprintf(lat_str, "%02d%07.4f", lat_d, lat_m);
    char lat_dir = (lat_e7 >= 0) ? 'N' : 'S';

    double lon_deg = lon_e7 / 10000000.0;
    int lon_d = (int)abs(lon_deg);
    double lon_m = (abs(lon_deg) - lon_d) * 60.0;
    char lon_str[15];
    sprintf(lon_str, "%03d%07.4f", lon_d, lon_m);
    char lon_dir = (lon_e7 >= 0) ? 'E' : 'W';

    // Format UTC time
    char time_str[12];
    sprintf(time_str, "%02d%02d%02d.00", utc_hour, utc_min, utc_sec);

    char date_str[8];
    int year_2digit = utc_year % 100;
    sprintf(date_str, "%02d%02d%02d", utc_day, utc_month, year_2digit);

    // Send RMC
    String rmc = "RMC,";
    rmc += String(time_str) + ",A,";
    rmc += String(lat_str) + "," + lat_dir + ",";
    rmc += String(lon_str) + "," + lon_dir + ",";
    rmc += "0.0,0.0,";
    rmc += String(date_str) + ",,,A";
    sendNMEASentence("GP", rmc);

    // Send GGA
    String gga = "GGA,";
    gga += String(time_str) + ",";
    gga += String(lat_str) + "," + lat_dir + ",";
    gga += String(lon_str) + "," + lon_dir + ",";
    gga += String(fix_type) + "," + String(sats) + ",1.0,";
    gga += String(alt, 1) + ",M,0.0,M,,";
    sendNMEASentence("GP", gga);

    // Send GSV for each GNSS
    if (sats_gps > 0) sendGSV("GP", sats_gps);
    if (sats_glonass > 0) sendGSV("GL", sats_glonass);
    if (sats_galileo > 0) sendGSV("GA", sats_galileo);
    if (sats_beidou > 0) sendGSV("GB", sats_beidou);

    last_gps = millis();
  }
}

void runPMSMock() {
  static uint32_t last_pms = 0;
  if(millis() - last_pms > 1000) {
    // Send 32-byte frame
    uint8_t buf[32] = {0x42,0x4D,0,28, 0,10,0,10,0,10, 0,10,0,10,0,10, 0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0};
    PMS_SERIAL.write(buf, 32);
    last_pms = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HITL Node D: I2C3 Dual B + UART Mock");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) esp_now_register_recv_cb(OnDataRecv);

  // Init Wires
  if(!Wire.begin(CM_ADDR, I2C0_SDA, I2C0_SCL)) Serial.println("Wire0 Fail");
  else { Wire.onReceive(onReceive0); Wire.onRequest(onRequest0); }

  if(!Wire1.begin(MCP_ADDR, I2C1_SDA, I2C1_SCL)) Serial.println("Wire1 Fail");
  else { Wire1.onReceive(onReceive1); Wire1.onRequest(onRequest1); }

  // Init UARTs
  GPS_SERIAL.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  PMS_SERIAL.begin(9600, SERIAL_8N1, PMS_RX_PIN, PMS_TX_PIN);
  
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  runGPSMock();
  runPMSMock();
  delay(10);
}
