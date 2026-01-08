// telemetry_rx_arduino_example.ino
//
// LoRa32 v2.1: Receive STM32 telemetry over UART, store to SD (actual values), transmit raw via LoRa
//
// Features:
// - UART RX from STM32 (50Hz, 20ms)
// - SD card logging (1 second interval, buffered) - CSV with actual values
// - LoRa transmission (5 second interval) - raw binary as received from UART
// - CRC16 verification
//
// Frame format matches Core/Inc/telemetry.h

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>

// ===== LoRa32 v2.1 Pin Configuration =====
// UART from STM32
#define STM32_RX_PIN 13   // GPIO13 (connected to STM32 TX)
#define STM32_TX_PIN 12   // GPIO12 (connected to STM32 RX)

// SD Card (LoRa32 v2.1 default SPI)
#define SD_CS_PIN 5

// LoRa (LoRa32 v2.1 fixed pins - SX1276)
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS   18
#define LORA_RST  23
#define LORA_IRQ  26

// LoRa frequency (Korea: 920.9 MHz)
#define LORA_FREQ 915E6

// ===== Timing Configuration =====
#define SD_WRITE_INTERVAL_MS   1000   // 1 second
#define LORA_TX_INTERVAL_MS    5000   // 5 seconds
#define FRAME_BUFFER_SIZE      60     // ~1.2 seconds at 50Hz

// ===== Telemetry Payload Structure (matches telemetry.h) =====
// This struct mirrors telemetry_payload_sensor_snapshot_t for parsing
#pragma pack(push, 1)
typedef struct {
    // 1. System Status
    uint32_t uptime_ms;
    uint16_t status_flags;
    uint16_t co2_ppm;

    // 2. IMU (x1000 scaled)
    int32_t accel_mps2_x1000[3];
    int32_t gyro_rads_x1000[3];

    // 3. Magnetometer
    float mag_uT[3];

    // 4. Temperature (x100 scaled)
    int16_t board_temp_c_x100;
    int16_t external_temp_c_x100;
    int16_t sht31_temp_c_x100;
    int16_t bat_temp_c_x100;

    // 5. GPS
    int32_t gps_lat_deg_e7;
    int32_t gps_lon_deg_e7;
    float gps_alt_m;
    uint8_t gps_fix;
    uint8_t gps_sats_used;
    uint8_t gps_sats_in_view_total;
    uint8_t gps_sats_in_view_gps;
    uint8_t gps_sats_in_view_glonass;
    uint8_t gps_sats_in_view_galileo;
    uint8_t gps_sats_in_view_beidou;
    
    // GPS UTC Time (from RMC sentence)
    uint8_t gps_utc_hour;
    uint8_t gps_utc_min;
    uint8_t gps_utc_sec;
    uint8_t gps_utc_day;
    uint8_t gps_utc_month;
    uint16_t gps_utc_year;

    uint16_t bat_mv;

    // 6. Air Quality
    uint16_t pm1_ugm3;
    uint16_t pm25_ugm3;
    uint16_t pm10_ugm3;
    int16_t ozone_ppb;

    // 7. Pressure / Humidity
    uint16_t sht31_rh_x100;
    uint32_t ms5611_press_pa;
    int16_t ms5611_temp_c_x100;

    // 8. Radiation
    uint16_t gdk101_usvh_x100;

    // 9. Heater Status
    uint8_t heater_bat_duty_percent;
    uint8_t heater_board_duty_percent;

    // 10. Altitude Fusion
    float press_alt_m;
    float kf_alt_m;
    float kf_roll_deg;
    float kf_pitch_deg;
} telemetry_payload_t;

typedef struct {
    uint8_t magic[2];      // {0xA5, 0x5A}
    uint8_t version;
    uint8_t msg_type;
    uint16_t payload_len;
    uint16_t seq;
    uint32_t timestamp_ms;
    telemetry_payload_t payload;
    uint16_t crc16;
} telemetry_frame_t;
#pragma pack(pop)

// ===== Helper Functions =====
static uint16_t u16le(const uint8_t *p) { 
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8); 
}

static uint32_t u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | 
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t i32le(const uint8_t *p) {
  return (int32_t)u32le(p);
}

static int16_t i16le(const uint8_t *p) {
  return (int16_t)u16le(p);
}

static float f32le(const uint8_t *p) {
  float val;
  memcpy(&val, p, sizeof(float));
  return val;
}

static uint16_t crc16_ccitt_false(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

// ===== Global State =====
static uint8_t rxFrame[256];
static size_t rxIdx = 0;
static size_t rxNeed = 0;

// Frame buffer for SD
static uint8_t frameBuffer[FRAME_BUFFER_SIZE][150];
static size_t frameLengths[FRAME_BUFFER_SIZE];
static int bufferCount = 0;

// Latest frame for LoRa (raw binary)
static uint8_t latestFrame[150];
static size_t latestFrameLen = 0;

// Timing
static uint32_t lastSdWrite = 0;
static uint32_t lastLoraTx = 0;

// Statistics
static uint32_t rxCount = 0;
static uint32_t crcErrors = 0;
static uint32_t sdWrites = 0;
static uint32_t loraTxCount = 0;

// SD file
static File logFile;
static char filename[32];

static void resetParser() {
  rxIdx = 0;
  rxNeed = 0;
}
#include <time.h>
#include <sys/time.h>

// Global GPS Time for SD timestamp (default 2026/1/8)
static uint16_t gpsYear = 2026;
static uint8_t gpsMonth = 1;
static uint8_t gpsDay = 8;
static uint8_t gpsHour = 0;
static uint8_t gpsMin = 0;
static uint8_t gpsSec = 0;

// Set ESP32 system time to match the GPS (KST) time
// This allows the SD library (via VFS) to use the correct timestamp for files
void syncInternalClock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;
    tm.tm_isdst = 0;
    
    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
}

// ===== SD Card Functions =====
void manageSDSpace() {
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  
  // Safety check
  if (total == 0) return;

  // Percentage threshold (e.g., 90% full) or fixed bytes (e.g., 50MB free)
  // Let's use 50MB free as safety margin
  const uint64_t MIN_FREE_BYTES = 50 * 1024 * 1024;
  
  while ((total - used) < MIN_FREE_BYTES) {
    Serial.printf("[SD] Low Space (Free: %llu MB). Cleaning up...\n", (total - used) / (1024*1024));
    
    File root = SD.open("/");
    if (!root) break;

    String oldestFile = "";
    int oldestIndex = -1;

    File entry = root.openNextFile();
    while (entry) {
      if (!entry.isDirectory()) {
        String name = entry.name();
        // Expect format: /telem_N.csv or telem_N.csv
        int pIndex = name.indexOf("telem_");
        int sIndex = name.indexOf(".csv");
        
        if (pIndex >= 0 && sIndex > pIndex) {
          String numStr = name.substring(pIndex + 6, sIndex);
          int num = numStr.toInt();
          
          if (oldestIndex == -1 || num < oldestIndex) {
            oldestIndex = num;
            oldestFile = name;
          }
        }
      }
      entry.close();
      entry = root.openNextFile();
    }
    root.close();

    if (oldestIndex != -1) {
      if (!oldestFile.startsWith("/")) oldestFile = "/" + oldestFile;
      Serial.printf("[SD] Deleting oldest: %s\n", oldestFile.c_str());
      SD.remove(oldestFile);
      
      // Update usage
      used = SD.usedBytes();
    } else {
      Serial.println("[SD] No valid log files found to delete. cleanup aborted.");
      break;
    }
  }
}

void createNewLogFile() {
  // SdFile::dateTimeCallback(dateTime) REMOVED - not supported by ESP32 SD lib
  // Instead, we rely on settimeofday() called when GPS data arrives.

  // Ensure space before creating new file
  manageSDSpace();

  // Generate unique sequential filename
  int fileIndex = 0;
  while (true) {
    snprintf(filename, sizeof(filename), "/telem_%d.csv", fileIndex);
    if (!SD.exists(filename)) {
      break;
    }
    fileIndex++;
  }
  
  logFile = SD.open(filename, FILE_WRITE);
  if (logFile) {
    // CSV header with ALL actual values including GPS UTC time
    logFile.println("rx_ms,seq,ts_ms,status,"
                    "gps_datetime,lat_deg,lon_deg,alt_m,fix,sats,sats_gps,sats_gl,sats_ga,sats_gb,"
                    "accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,"
                    "mag_x,mag_y,mag_z,"
                    "board_temp,ext_temp,sht_temp,bat_temp,"
                    "press_pa,press_temp,humidity,"
                    "co2_ppm,pm1,pm25,pm10,ozone_ppb,radiation,"
                    "bat_mv,heater_bat,heater_board,"
                    "press_alt,kf_alt,kf_roll,kf_pitch");
    logFile.flush();
    Serial.printf("[SD] Created: %s\n", filename);
  } else {
    Serial.println("[SD] Failed to create file!");
  }
}

void writeBufferToSd() {
  if (!logFile) return;
  
  for (int i = 0; i < bufferCount; i++) {
    uint8_t *f = frameBuffer[i];
    size_t len = frameLengths[i];
    
    if (len < 60) continue; // Too short
    
    // Parse header
    uint16_t seq = u16le(&f[6]);
    uint32_t ts_ms = u32le(&f[8]);
    
    // Payload starts at offset 12
    const uint8_t *p = &f[12];
    
    // Parse all payload fields with actual values
    uint32_t uptime = u32le(p);        // offset 0
    uint16_t status = u16le(p + 4);    // offset 4
    uint16_t co2_ppm = u16le(p + 6);   // offset 6
    
    // IMU (x1000 scaled -> convert to actual)
    float accel_x = i32le(p + 8) / 1000.0f;
    float accel_y = i32le(p + 12) / 1000.0f;
    float accel_z = i32le(p + 16) / 1000.0f;
    float gyro_x = i32le(p + 20) / 1000.0f;
    float gyro_y = i32le(p + 24) / 1000.0f;
    float gyro_z = i32le(p + 28) / 1000.0f;
    
    // Magnetometer (already float)
    float mag_x = f32le(p + 32);
    float mag_y = f32le(p + 36);
    float mag_z = f32le(p + 40);
    
    // Temperature (x100 scaled -> convert to actual)
    float board_temp = i16le(p + 44) / 100.0f;
    float ext_temp = i16le(p + 46) / 100.0f;
    float sht_temp = i16le(p + 48) / 100.0f;
    float bat_temp = i16le(p + 50) / 100.0f;
    
    // GPS (lat/lon x10^7 -> convert to actual degrees)
    double lat_deg = i32le(p + 52) / 10000000.0;
    double lon_deg = i32le(p + 56) / 10000000.0;
    float gps_alt = f32le(p + 60);
    uint8_t gps_fix = p[64];
    uint8_t gps_sats = p[65];
    uint8_t gps_sats = p[65];
    // sat_view fields: 66-72 (7 bytes)
    // Offset 67=total, 68=gps, 69=glo, 70=gal, 71=bei
    uint8_t sats_gps = p[68];
    uint8_t sats_gl = p[69];
    uint8_t sats_ga = p[70];
    uint8_t sats_gb = p[71];
    
    // GPS UTC Time (offset 73-79: hour, min, sec, day, month, year[2])
    uint8_t utc_hour = p[73];
    uint8_t utc_min = p[74];
    uint8_t utc_sec = p[75];
    uint8_t utc_day = p[76];
    uint8_t utc_month = p[77];
    uint16_t utc_year = u16le(p + 78);

    // Convert to KST (UTC+9)
    if (utc_year > 2020) {
      utc_hour += 9;
      if (utc_hour >= 24) {
        utc_hour -= 24;
        utc_day++;
        
        uint8_t dim = 31;
        if (utc_month == 4 || utc_month == 6 || utc_month == 9 || utc_month == 11) dim = 30;
        else if (utc_month == 2) {
          if ((utc_year % 4 == 0 && utc_year % 100 != 0) || (utc_year % 400 == 0)) dim = 29;
          else dim = 28;
        }
        
        if (utc_day > dim) {
          utc_day = 1;
          utc_month++;
          if (utc_month > 12) {
            utc_month = 1;
            utc_year++;
          }
        }
      }
    }
    
    // Battery voltage (offset 80)
    uint16_t bat_mv = u16le(p + 80);
    
    // Air Quality (offset 82-89)
    uint16_t pm1 = u16le(p + 82);
    uint16_t pm25 = u16le(p + 84);
    uint16_t pm10 = u16le(p + 86);
    int16_t ozone = i16le(p + 88);
    
    // Pressure / Humidity (offset 90-97)
    float humidity = u16le(p + 90) / 100.0f;
    uint32_t press_pa = u32le(p + 92);
    float press_temp = i16le(p + 96) / 100.0f;
    
    // Radiation (offset 98)
    float radiation = u16le(p + 98) / 100.0f;
    
    // Heater (offset 100-101)
    uint8_t heater_bat = p[100];
    uint8_t heater_board = p[101];
    
    // Altitude Fusion (offset 102-117, already floats)
    float press_alt = f32le(p + 102);
    float kf_alt = f32le(p + 106);
    float kf_roll = f32le(p + 110);
    float kf_pitch = f32le(p + 114);
    
    // Write CSV line with actual values including GPS datetime
    logFile.printf("%lu,%u,%lu,0x%04X,",
                   millis(), seq, ts_ms, status);
    // GPS datetime in ISO 8601 format: YYYY-MM-DDTHH:MM:SS
    logFile.printf("%04u-%02u-%02uT%02u:%02u:%02u,",
                   utc_year, utc_month, utc_day, utc_hour, utc_min, utc_sec);
    logFile.printf("%.7f,%.7f,%.2f,%u,%u,%u,%u,%u,%u,",
                   lat_deg, lon_deg, gps_alt, gps_fix, gps_sats, sats_gps, sats_gl, sats_ga, sats_gb);
    logFile.printf("%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,",
                   accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
    logFile.printf("%.2f,%.2f,%.2f,",
                   mag_x, mag_y, mag_z);
    logFile.printf("%.2f,%.2f,%.2f,%.2f,",
                   board_temp, ext_temp, sht_temp, bat_temp);
    logFile.printf("%lu,%.2f,%.2f,",
                   press_pa, press_temp, humidity);
    logFile.printf("%u,%u,%u,%u,%d,%.2f,",
                   co2_ppm, pm1, pm25, pm10, ozone, radiation);
    logFile.printf("%u,%u,%u,",
                   bat_mv, heater_bat, heater_board);
    logFile.printf("%.2f,%.2f,%.2f,%.2f\n",
                   press_alt, kf_alt, kf_roll, kf_pitch);
  }
  
  logFile.flush();
  bufferCount = 0;
  sdWrites++;
}

// ===== LoRa Functions =====
// Send raw binary frame as received from UART
void sendLoRaPacket() {
  if (latestFrameLen == 0) return;
  
  // CSMA / LBT (Listen Before Talk)
  // Check if we are currently receiving a packet.
  // parsePacket() returns the size of the packet that is waiting to be read.
  // If > 0, the channel is likely busy or we just received something.
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
      Serial.printf("[LoRa] Channel Busy (RX %d bytes), Skipping TX to avoid collision\n", packetSize);
      // We skip transmission this cycle. The data will be attempted again 
      // in the next cycle (if it's still "latestFrame").
      // Ideally, we should also process this incoming packet, but since this
      // loop() is mainly for RX, it will be picked up in the main loop iteration.
      return;
  }
  
  // Channel is free, proceed to transmit
  LoRa.beginPacket();
  LoRa.write(latestFrame, latestFrameLen);
  LoRa.endPacket();

  // Switch back to RX mode immediately (though generic loop handles it)
  LoRa.receive();

  loraTxCount++;
  
  uint16_t seq = u16le(&latestFrame[6]);
  Serial.printf("[LoRa] TX raw seq=%u len=%u bytes\n", seq, latestFrameLen);
}

// ===== Frame Processing =====
void processFrame(uint8_t *frame, size_t len) {
  const uint16_t payload_len = u16le(&frame[4]);
  const uint16_t seq = u16le(&frame[6]);
  const uint32_t ts_ms = u32le(&frame[8]);
  
  // CRC check
  const uint16_t crc_rx = u16le(&frame[12 + payload_len]);
  const uint16_t crc_calc = crc16_ccitt_false(frame, 12 + payload_len);
  
  if (crc_rx != crc_calc) {
    crcErrors++;
    Serial.printf("[RX] CRC ERROR seq=%u\n", seq);
    return;
  }
  
  rxCount++;
  
  // Store to buffer for SD
  if (bufferCount < FRAME_BUFFER_SIZE) {
    memcpy(frameBuffer[bufferCount], frame, len);
    frameLengths[bufferCount] = len;
    bufferCount++;
  }
  
  // Keep latest for LoRa (raw binary as-is)
  memcpy(latestFrame, frame, len);
  latestFrameLen = len;
  
  // Update Global GPS Time for SD timestamp
  // GPS fields are at offset 52 (lat) ... 73 (hour)
  // Check GPS fix (offset 64)
  const uint8_t gps_fix = frame[64 + 12]; // Offset 12 is payload start, 64 is offset inside payloadStruct
  
  // Actually, we can just use the memory directly. 
  // Frame layout: [Header 12] [Payload...] [CRC 2]
  // Payload offset = 12
  // UTC Date/Time offsets in payload:
  // Hour: 73, Min: 74, Sec: 75, Day: 76, Month: 77, Year: 78 (uint16)
  
  const uint8_t *p = &frame[12];
  uint16_t year = u16le(p + 78);
  
  // Basic validation (Year > 2020) guarantees we have some time set
  if (year > 2020) {
      // Convert UTC to KST (UTC+9) for SD Timestamp
      // 1. Get UTC values
      uint16_t t_year = year;
      uint8_t t_month = p[77];
      uint8_t t_day = p[76];
      uint8_t t_hour = p[73];
      uint8_t t_min = p[74];
      uint8_t t_sec = p[75];

      // 2. Add 9 hours
      t_hour += 9;
      
      // 3. Handle rollover
      if (t_hour >= 24) {
        t_hour -= 24;
        t_day++;
        
        // Days in month calculation
        uint8_t daysInMonth = 31;
        if (t_month == 4 || t_month == 6 || t_month == 9 || t_month == 11) {
          daysInMonth = 30;
        } else if (t_month == 2) {
          // Leap year check
          if ((t_year % 4 == 0 && t_year % 100 != 0) || (t_year % 400 == 0)) {
            daysInMonth = 29;
          } else {
            daysInMonth = 28;
          }
        }
        
        if (t_day > daysInMonth) {
          t_day = 1;
          t_month++;
          if (t_month > 12) {
            t_month = 1;
            t_year++;
          }
        }
      }

      // 4. Update Globals
      gpsYear = t_year;
      gpsMonth = t_month;
      gpsDay = t_day;
      gpsHour = t_hour;
      gpsMin = t_min;
      gpsSec = t_sec;
      
      // 5. Update System Clock for SD File Timestamps
      syncInternalClock(gpsYear, gpsMonth, gpsDay, gpsHour, gpsMin, gpsSec);
  }
  
  // Debug output (every 50 frames = 1 second)
  if (rxCount % 50 == 0) {
    Serial.printf("[RX] seq=%u ts=%lu status=0x%04X buf=%d\n", 
                  seq, ts_ms, u16le(&frame[16]), bufferCount);
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== LoRa32 v2.1 Telemetry Receiver ===");
  
  // UART from STM32 (버퍼 확장: SD 쓰기 중 데이터 손실 방지)
  Serial2.setRxBufferSize(1024);  // 8프레임(160ms) 분량
  Serial2.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  Serial.printf("[UART] RX=%d TX=%d\n", STM32_RX_PIN, STM32_TX_PIN);
  
  // SD Card
  if (SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] Initialized");
    createNewLogFile();
  } else {
    Serial.println("[SD] Init FAILED!");
  }
  
  // LoRa (LoRa32 v2.1 uses SPI with fixed pins)
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  
  if (LoRa.begin(LORA_FREQ)) {
    LoRa.setSpreadingFactor(11);
    LoRa.setSignalBandwidth(125E3);
    LoRa.setCodingRate4(5);
    Serial.printf("[LoRa] Initialized at %.1f MHz\n", LORA_FREQ / 1E6);
  } else {
    Serial.println("[LoRa] Init FAILED!");
  }
  
  lastSdWrite = millis();
  lastLoraTx = millis();
}

// ===== Main Loop =====
void loop() {
  uint32_t now = millis();
  
  // Read UART
  while (Serial2.available() > 0) {
    const uint8_t b = (uint8_t)Serial2.read();
    
    // Sync on magic bytes
    if (rxIdx == 0) {
      if (b == 0xA5) rxFrame[rxIdx++] = b;
      continue;
    }
    if (rxIdx == 1) {
      if (b == 0x5A) rxFrame[rxIdx++] = b;
      else resetParser();
      continue;
    }
    
    rxFrame[rxIdx++] = b;
    
    // Header complete - get expected length
    if (rxIdx == 12) {
      const uint16_t payload_len = u16le(&rxFrame[4]);
      rxNeed = 12 + (size_t)payload_len + 2;  // + CRC16
      if (rxNeed > sizeof(rxFrame)) {
        resetParser();
      }
      continue;
    }
    
    // Frame complete
    if (rxNeed && rxIdx == rxNeed) {
      processFrame(rxFrame, rxIdx);
      resetParser();
    }
    
    // Overflow protection
    if (rxIdx >= sizeof(rxFrame)) {
      resetParser();
    }
  }
  
  // SD write (1 second)
  if (now - lastSdWrite >= SD_WRITE_INTERVAL_MS) {
    writeBufferToSd();
    lastSdWrite = now;
  }
  
  // LoRa transmit (5 seconds) - sends raw binary frame
  if (now - lastLoraTx >= LORA_TX_INTERVAL_MS) {
    sendLoRaPacket();
    lastLoraTx = now;
  }
}
