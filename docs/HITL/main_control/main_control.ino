#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "hitl_protocol.h"

// --- Main Control Configuration ---
// Function: 
// 1. Receive 'ALL:...' Simulation Data from PC (Serial)
// 2. Broadcast HitlStatePacket via ESP-NOW to all Mock Nodes
// 3. Receive STM32 Telemetry via UART (RX_PIN) and pass to PC? Or just log.
//    User said "receive data from STM32 UART3"

#define STM32_UART3_RX_PIN 16 // Adjust pin for ESP32 UART2 RX
#define STM32_UART3_TX_PIN 17 // Not used if only listening

HardwareSerial SerialSTM(2);

HitlStatePacket sim_state;

// Broadcast Address (All FF)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Debug
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Handle Feedback from Mocks (Heaters, Resets) if implemented
  if (len == sizeof(HitlFeedbackPacket)) {
    HitlFeedbackPacket *fb = (HitlFeedbackPacket*)incomingData;
    if (fb->reset_flags > 0) {
      Serial.printf("[FEEDBACK] Sensor RESET Detected on Node %d! Flags: %02X\n", fb->node_id, fb->reset_flags);
    }
    if (fb->heater_bat_duty > 0 || fb->heater_bd_duty > 0) {
      // Log heater status
      // Serial.printf("[FEEDBACK] Heaters: Bat %d%%, Bd %d%%\n", fb->heater_bat_duty, fb->heater_bd_duty);
    }
  }
}

void setup() {
  Serial.begin(115200);
  // STM32 UART3 is configured for 115200 (Checked in usart.c)
  SerialSTM.begin(115200, SERIAL_8N1, STM32_UART3_RX_PIN, STM32_UART3_TX_PIN);

  Serial.println("HITL Main Control Node Starting...");

  // Init WiFi Mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register Peer (Broadcast)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("ESP-NOW Broadcast Active.");
}

void parseSimData(String input) {
  // PC sends "ALL:Time,Lat,Lon,Alt,..." (CSV)
  if (input.startsWith("ALL:")) {
     // Expected format: ALL:Timestamp,Status,CO2,ax,ay,az,gx,gy,gz,mx,my,mz,t_brd,t_ext,t_sht,t_bat,lat,lon,alt,... 
     // Using ParseInt/Float from String is slow but easiest. Converting to C-string for strtok is better.
     
     char buf[256];
     input.toCharArray(buf, 256);
     
     char *ptr = strtok(buf, ":"); // Skip "ALL"
     ptr = strtok(NULL, ","); // Timestamp
     if (ptr) sim_state.timestamp_ms = atol(ptr);
     
     ptr = strtok(NULL, ","); // Status (Skip)
     ptr = strtok(NULL, ","); // CO2
     if (ptr) sim_state.co2 = atoi(ptr);
     
     // Accel (x,y,z)
     for (int i=0; i<3; i++) { ptr = strtok(NULL, ","); if(ptr) sim_state.accel[i] = atoi(ptr)/1000.0; }
     
     // Gyro (x,y,z)
     for (int i=0; i<3; i++) { ptr = strtok(NULL, ","); if(ptr) sim_state.gyro[i] = atoi(ptr)/1000.0; }
     
     // Mag (x,y,z)
     for (int i=0; i<3; i++) { ptr = strtok(NULL, ","); if(ptr) sim_state.mag[i] = atoi(ptr); } // uT matches?
     
     // Temps: Board, Ext, SHT, Bat
     ptr = strtok(NULL, ","); if(ptr) sim_state.temp_c = atoi(ptr)/100.0; // Use Board/Ambient
     ptr = strtok(NULL, ","); if(ptr) sim_state.ext_temp_c = atoi(ptr)/100.0;
     ptr = strtok(NULL, ","); // SHT (Skip/Reuse)
     ptr = strtok(NULL, ","); // Bat Temp (Skip)
     
     // GPS: Lat, Lon, Alt
     ptr = strtok(NULL, ","); if(ptr) sim_state.lat_e7 = atol(ptr);
     ptr = strtok(NULL, ","); if(ptr) sim_state.lon_e7 = atol(ptr);
     ptr = strtok(NULL, ","); if(ptr) sim_state.alt_m = atof(ptr);
     
     // Fix, Sats... (Skip 7 items)
     for(int i=0; i<7; i++) strtok(NULL, ",");
     
     // Time (Skip 6 items)
     for(int i=0; i<6; i++) strtok(NULL, ",");
     
     // BatMV
     ptr = strtok(NULL, ","); if(ptr) sim_state.bat_mv = atoi(ptr);
     
     // Air: PM1, PM2.5, PM10, Ozone
     ptr = strtok(NULL, ","); // PM1
     ptr = strtok(NULL, ","); if(ptr) sim_state.pm2_5 = atoi(ptr);
     ptr = strtok(NULL, ","); // PM10
     ptr = strtok(NULL, ","); if(ptr) sim_state.ozone_ppb = atoi(ptr);
     
     // Env: RH, Press, Temp
     ptr = strtok(NULL, ","); if(ptr) sim_state.humidity = atoi(ptr)/100.0;
     ptr = strtok(NULL, ","); if(ptr) sim_state.pressure_pa = atol(ptr); // Pa
     ptr = strtok(NULL, ","); // Temp (Skip)
     
     // Rad
     ptr = strtok(NULL, ","); if(ptr) sim_state.radiation = atoi(ptr)/100.0;
     
     // Send Immediately
     esp_now_send(broadcastAddress, (uint8_t *) &sim_state, sizeof(sim_state));
  }
  else if (input.startsWith("CMD,FAULT,")) {
    // Format: CMD,FAULT,COMP,TYPE,DURATION
    // Example: CMD,FAULT,GPS,TIMEOUT,10
    
    // Simple Parse
    int first = input.indexOf(',', 10);
    int second = input.indexOf(',', first + 1);
    
    if (first > 0 && second > 0) {
       String compStr = input.substring(10, first);
       String typeStr = input.substring(first + 1, second);
       
       uint8_t compId = 0;
       if (compStr == "GPS") compId = 1;
       else if (compStr == "IMU") compId = 2;
       else if (compStr == "BARO") compId = 3;
       else if (compStr == "ENV") compId = 4;
       else if (compStr == "CO2") compId = 5;
       else if (compStr == "RAD") compId = 6;
       
       uint8_t typeId = 0;
       if (typeStr == "TIMEOUT") typeId = 1;
       else if (typeStr == "FREEZE") typeId = 2;
       else if (typeStr == "NOISE") typeId = 3;
       else if (typeStr == "OFFSET") typeId = 4;
       else if (typeStr == "FAIL") typeId = 5;
       else if (typeStr == "HIGH") typeId = 6; // Reuse
       
       sim_state.fault_comp = compId;
       sim_state.fault_type = typeId;
       
       Serial.printf("[CMD] Fault Injected: Comp=%d Type=%d\n", compId, typeId);
       
       // Force immediate broadcast
       esp_now_send(broadcastAddress, (uint8_t *) &sim_state, sizeof(sim_state));
    }
  }
}

// --- STM32 Telemetry Parser ---
// Structure from telemetry.h
typedef struct __attribute__((packed)) {
    uint8_t magic[2];      /* {0xA5, 0x5A} */
    uint8_t version;       /* 1 */
    uint8_t msg_type;      /* 0x01 heartbeat, 0x02 sensor snapshot */
    uint16_t payload_len;  /* bytes */
    uint16_t seq;
    uint32_t timestamp_ms;
    // Payload follows...
} TelemHeader;

enum TelemState {
  WAIT_SYNC1, WAIT_SYNC2, READ_HEADER, READ_PAYLOAD, READ_CRC
};

TelemState t_state = WAIT_SYNC1;
uint8_t t_buf[256];
uint16_t t_idx = 0;
TelemHeader t_hdr;
uint16_t t_payload_remain = 0;

void processSerialSTM() {
  while (SerialSTM.available()) {
    uint8_t b = SerialSTM.read();
    
    switch (t_state) {
      case WAIT_SYNC1:
        if (b == 0xA5) t_state = WAIT_SYNC2;
        break;
        
      case WAIT_SYNC2:
        if (b == 0x5A) {
          t_state = READ_HEADER;
          t_idx = 0;
          // Store magic
          t_buf[t_idx++] = 0xA5;
          t_buf[t_idx++] = 0x5A;
        } else {
          t_state = WAIT_SYNC1; // Reset
        }
        break;
        
      case READ_HEADER:
        t_buf[t_idx++] = b;
        if (t_idx >= sizeof(TelemHeader)) {
          memcpy(&t_hdr, t_buf, sizeof(TelemHeader));
          if (t_hdr.payload_len > 200) { // Safety check
             t_state = WAIT_SYNC1;
          } else {
             t_payload_remain = t_hdr.payload_len;
             t_state = READ_PAYLOAD;
          }
        }
        break;
        
      case READ_PAYLOAD:
        t_buf[t_idx++] = b;
        t_payload_remain--;
        if (t_payload_remain == 0) {
          t_state = READ_CRC;
        }
        break;
        
      case READ_CRC:
        t_buf[t_idx++] = b; // CRC Byte 1
        if (t_idx >= sizeof(TelemHeader) + t_hdr.payload_len + 2) {
           // Frame Complete
           
           // Output HEX for PC Parser
           // Format: TELEM_HEX:[HEX_STRING]\n
           Serial.print("TELEM_HEX:");
           // Header + Payload + CRC
           for (int i=0; i < t_idx; i++) {
             Serial.printf("%02X", t_buf[i]);
           }
           Serial.println();
           
           t_state = WAIT_SYNC1;
        }
        break;
    }
  }
}

void loop() {
  // 1. Handle PC Input (Simulation Scenarios)
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    parseSimData(line);
  }
  
  // 2. Handle STM32 Telemetry
  processSerialSTM();
  
  // Auto-generate test wave if no PC input
  static uint32_t last_sim = 0;
  if (millis() - last_sim > 100) { // 10Hz
     sim_state.timestamp_ms = millis();
     sim_state.alt_m += 0.1;
     if (sim_state.alt_m > 1000) sim_state.alt_m = 0;
     
     // Mock defaults to prevent zeros
     if (sim_state.pressure_pa == 0) sim_state.pressure_pa = 101325;
     if (sim_state.temp_c == 0) sim_state.temp_c = 25.0;
     
     esp_now_send(broadcastAddress, (uint8_t *) &sim_state, sizeof(sim_state));
     last_sim = millis();
  }
}
