#ifndef HITL_PROTOCOL_H
#define HITL_PROTOCOL_H

#include <stdint.h>

// ESP-NOW uses a fixed channel (e.g. 1)
#define ESP_NOW_CHANNEL 1

// Data Structure broadcasted by Main Control to all Mocks
typedef struct __attribute__((packed)) {
  // Timestamp
  uint32_t timestamp_ms;

  // GPS Data
  int32_t lat_e7;
  int32_t lon_e7;
  float   alt_m;
  uint8_t fix_type;
  uint8_t sats;

  // Environment
  float   temp_c;      // SHT31, MS5611, DS18B20 (Internal/Ambient)
  float   ext_temp_c;  // MCP9600 (Thermocouple)
  float   pressure_pa; // MS5611
  float   humidity;    // SHT31
  
  // IMU / Orientation
  float   accel[3]; // m/s^2
  float   gyro[3];  // rad/s
  float   mag[3];   // uT

  // Air Quality
  uint16_t pm2_5;
  uint16_t co2;
  int16_t  ozone_ppb;
  float    radiation;

} HitlStatePacket;

// Command Structure (Optional, for Feedback from Mocks to Main)
typedef struct __attribute__((packed)) {
  uint8_t node_id; // 0=GPIO, 1=UART, etc.
  uint8_t msg_type; 
  // Heater Status
  uint8_t heater_bat_duty;
  uint8_t heater_bd_duty;
  // Reset Events
  uint8_t reset_flags; 
} HitlFeedbackPacket;

#endif
