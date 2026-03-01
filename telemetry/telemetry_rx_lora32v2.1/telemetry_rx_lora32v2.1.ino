/**
 * @file telemetry_rx_lora32v2.1.ino
 * @brief LoRa32 v2.1 텔레메트리 수신기 (STM32 연동)
 * @details STM32로부터 UART로 텔레메트리 데이터를 수신하여 SD 카드에 저장하고, LoRa로 지상국에 재전송합니다.
 *
 * @section features 주요 기능
 * - STM32로부터 UART 수신 (50Hz, 20ms 주기)
 * - SD 카드 로깅 (1초 간격, 버퍼링 사용) - 실제 값으로 변환하여 CSV 저장
 * - LoRa 전송 (5초 간격) - UART로 수신된 바이너리 원본 데이터 전송
 * - CRC16 무결성 검증
 *
 * @note 프레임 포맷은 Core/Inc/telemetry.h와 일치해야 합니다.
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>

// ===== LoRa32 v2.1 핀 설정 =====
// STM32 연결 UART
#define STM32_RX_PIN 13   /**< GPIO13 (STM32 TX와 연결) */
#define STM32_TX_PIN 12   /**< GPIO12 (STM32 RX와 연결) */

// SD 카드 (LoRa32 v2.1 기본 SPI)
#define SD_CS_PIN 5

// LoRa 모듈 (LoRa32 v2.1 고정 핀 - SX1276)
#define LORA_SCK  5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS   18
#define LORA_RST  23
#define LORA_IRQ  26

// LoRa 주파수 (한국: 920.9 MHz 등, 여기선 915MHz 대역 사용 예시)
#define LORA_FREQ 915E6

// ===== 타이밍 설정 =====
#define SD_WRITE_INTERVAL_MS   1000   /**< SD 카드 쓰기 주기 (1초) */
#define LORA_TX_INTERVAL_MS    5000   /**< LoRa 전송 주기 (5초) */
#define FRAME_BUFFER_SIZE      60     /**< 프레임 버퍼 크기 (50Hz 기준 약 1.2초 분량) */

// ===== 텔레메트리 페이로드 구조체 (telemetry.h와 일치) =====
/** 
 * @struct telemetry_payload_t
 * @brief 파싱을 위한 텔레메트리 데이터 구조체 (Packed)
 * @details telemetry.h의 telemetry_payload_sensor_snapshot_t와 동일한 구조
 */
#pragma pack(push, 1)
typedef struct {
    // 1. 시스템 상태
    uint32_t uptime_ms;             /**< 시스템 가동 시간 (ms) */
    uint16_t status_flags;          /**< 상태 플래그 */
    uint16_t co2_ppm;               /**< CO2 농도 (PPM) */

    // 2. IMU (x1000 스케일링)
    int32_t accel_mps2_x1000[3];    /**< 가속도 (m/s^2 * 1000) */
    int32_t gyro_rads_x1000[3];     /**< 자이로 (rad/s * 1000) */

    // 3. 자력계
    float mag_uT[3];                /**< 자계 강도 (uT) */

    // 4. 온도 (x100 스케일링)
    int16_t board_temp_c_x100;      /**< 보드 온도 */
    int16_t external_temp_c_x100;   /**< 외부 온도 (열전대) */
    int16_t sht31_temp_c_x100;      /**< SHT31 온도 */
    int16_t bat_temp_c_x100;        /**< 배터리 온도 */

    // 5. GPS
    int32_t gps_lat_deg_e7;         /**< 위도 (도 * 10^7) */
    int32_t gps_lon_deg_e7;         /**< 경도 (도 * 10^7) */
    float gps_alt_m;                /**< 고도 (m) */
    uint8_t gps_fix;                /**< Fix 유형 (0=No, 2=2D, 3=3D) */
    uint8_t gps_sats_used;          /**< 사용된 위성 수 */
    uint8_t gps_sats_in_view_total; /**< 보이는 총 위성 수 */
    uint8_t gps_sats_in_view_gps;   /**< 보이는 GPS 위성 수 */
    uint8_t gps_sats_in_view_glonass; /**< 보이는 GLONASS 위성 수 */
    uint8_t gps_sats_in_view_galileo; /**< 보이는 Galileo 위성 수 */
    uint8_t gps_sats_in_view_beidou;  /**< 보이는 BeiDou 위성 수 */
    
    // GPS UTC 시간 (RMC 문장 기반)
    uint8_t gps_utc_hour;
    uint8_t gps_utc_min;
    uint8_t gps_utc_sec;
    uint8_t gps_utc_day;
    uint8_t gps_utc_month;
    uint16_t gps_utc_year;

    uint16_t bat_mv;                /**< 배터리 전압 (mV) */

    // 6. 대기질 (Air Quality)
    uint16_t pm1_ugm3;              /**< 미세먼지 PM1.0 (ug/m3) */
    uint16_t pm25_ugm3;             /**< 미세먼지 PM2.5 (ug/m3) */
    uint16_t pm10_ugm3;             /**< 미세먼지 PM10 (ug/m3) */
    int16_t ozone_ppb;              /**< 오존 농도 (ppb) */

    // 7. 기압 / 습도
    uint16_t sht31_rh_x100;         /**< 습도 (% * 100) */
    uint32_t ms5611_press_pa;       /**< 기압 (Pa) */
    int16_t ms5611_temp_c_x100;     /**< 기압센서 온도 */

    // 8. 방사선
    uint16_t gdk101_usvh_x100;      /**< 방사선량 (uSv/h * 100) */

    // 9. 히터 상태
    uint8_t heater_bat_duty_percent;   /**< 배터리 히터 듀티비 (%) */
    uint8_t heater_board_duty_percent; /**< 보드 히터 듀티비 (%) */

    // 10. 고도 융합 (Altitude Fusion)
    float press_alt_m;   /**< 기압 고도 (m) */
    float kf_alt_m;      /**< 칼만 필터 추정 고도 (m) */
    float kf_roll_deg;   /**< 롤 각도 (deg) */
    float kf_pitch_deg;  /**< 피치 각도 (deg) */
} telemetry_payload_t;

/** 
 * @struct telemetry_frame_t
 * @brief 전체 텔레메트리 프레임 구조
 */
typedef struct {
    uint8_t magic[2];      /**< 매직 바이트 {0xA5, 0x5A} */
    uint8_t version;       /**< 프로토콜 버전 */
    uint8_t msg_type;      /**< 메시지 유형 */
    uint16_t payload_len;  /**< 페이로드 길이 */
    uint16_t seq;          /**< 시퀀스 번호 */
    uint32_t timestamp_ms; /**< 타임스탬프 (ms) */
    telemetry_payload_t payload; /**< 페이로드 데이터 */
    uint16_t crc16;        /**< CRC16 체크섬 (헤더+페이로드) */
} telemetry_frame_t;
#pragma pack(pop)

// ===== 유틸리티(Helper) 함수 =====
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

/** @brief CRC16-CCITT (False) 계산 함수 */
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

// ===== 전역 상태 변수 =====
static uint8_t rxFrame[256];
static size_t rxIdx = 0;
static size_t rxNeed = 0;

// SD 저장용 프레임 버퍼
static uint8_t frameBuffer[FRAME_BUFFER_SIZE][150];
static size_t frameLengths[FRAME_BUFFER_SIZE];
static int bufferCount = 0;

// LoRa 전송용 최신 프레임 (Raw Binary)
static uint8_t latestFrame[150];
static size_t latestFrameLen = 0;

// 타이밍 관리
static uint32_t lastSdWrite = 0;
static uint32_t lastLoraTx = 0;

// 통계 변수
static uint32_t rxCount = 0;
static uint32_t crcErrors = 0;
static uint32_t sdWrites = 0;
static uint32_t loraTxCount = 0;

// SD 파일 객체
static File logFile;
static char filename[32];

static void resetParser() {
  rxIdx = 0;
  rxNeed = 0;
}
#include <time.h>
#include <sys/time.h>

// SD 타임스탬프용 GPS 시간 (기본값: 2026/1/8)
static uint16_t gpsYear = 2026;
static uint8_t gpsMonth = 1;
static uint8_t gpsDay = 8;
static uint8_t gpsHour = 0;
static uint8_t gpsMin = 0;
static uint8_t gpsSec = 0;

/** 
 * @brief ESP32 내부 시스템 시간을 GPS 시간(KST)으로 동기화
 * @details 이를 통해 SD 라이브러리(VFS)가 파일 생성/수정 시 올바른 시간을 사용하도록 합니다.
 */
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

// ===== SD 카드 관련 함수 =====

/** 
 * @brief SD 카드 여유 공간 관리
 * @details 여유 공간이 부족할 경우 가장 오래된 로그 파일을 삭제합니다.
 */
void manageSDSpace() {
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  
  // 안전 검사
  if (total == 0) return;

  // 임계값 설정 (예: 50MB 여유 공간 확보)
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
        // 파일명 형식 예상: /telem_N.csv 또는 telem_N.csv
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
      
      // 사용량 업데이트
      used = SD.usedBytes();
    } else {
      Serial.println("[SD] No valid log files found to delete. cleanup aborted.");
      break;
    }
  }
}

/** 
 * @brief 새로운 로그 파일 생성
 * @details 순차적인 번호를 부여하여 새 CSV 파일을 생성합니다.
 */
void createNewLogFile() {
  // ESP32 SD 라이브러리는 dateTimeCallback을 지원하지 않음
  // 대신 GPS 시간 수신 시 settimeofday()를 호출하여 시스템 시간을 동기화함

  // 파일 생성 전 공간 확보
  manageSDSpace();

  // 중복되지 않는 순차 파일명 생성
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
    // CSV 헤더 작성 (GPS UTC 시간 포함 모든 실제 값)
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

/** 
 * @brief 프레임 버퍼 내용을 SD 카드에 기록
 * @details 바이너리 데이터를 파싱하여 사람이 읽을 수 있는 CSV 형식으로 변환 후 저장합니다.
 */
void writeBufferToSd() {
  if (!logFile) return;
  
  for (int i = 0; i < bufferCount; i++) {
    uint8_t *f = frameBuffer[i];
    size_t len = frameLengths[i];
    
    if (len < 60) continue; // 너무 짧으면 무시
    
    // 헤더 파싱
    uint16_t seq = u16le(&f[6]);
    uint32_t ts_ms = u32le(&f[8]);
    
    // 페이로드는 오프셋 12부터 시작
    const uint8_t *p = &f[12];
    
    // 모든 페이로드 필드를 실제 값으로 파싱
    uint32_t uptime = u32le(p);        // offset 0
    uint16_t status = u16le(p + 4);    // offset 4
    uint16_t co2_ppm = u16le(p + 6);   // offset 6
    
    // IMU (x1000 스케일 -> 실제 값 변환)
    float accel_x = i32le(p + 8) / 1000.0f;
    float accel_y = i32le(p + 12) / 1000.0f;
    float accel_z = i32le(p + 16) / 1000.0f;
    float gyro_x = i32le(p + 20) / 1000.0f;
    float gyro_y = i32le(p + 24) / 1000.0f;
    float gyro_z = i32le(p + 28) / 1000.0f;
    
    // 자력계 (이미 float)
    float mag_x = f32le(p + 32);
    float mag_y = f32le(p + 36);
    float mag_z = f32le(p + 40);
    
    // 온도 (x100 스케일 -> 실제 값 변환)
    float board_temp = i16le(p + 44) / 100.0f;
    float ext_temp = i16le(p + 46) / 100.0f;
    float sht_temp = i16le(p + 48) / 100.0f;
    float bat_temp = i16le(p + 50) / 100.0f;
    
    // GPS (위경도 x10^7 -> 실제 도(degree) 단위 변환)
    double lat_deg = i32le(p + 52) / 10000000.0;
    double lon_deg = i32le(p + 56) / 10000000.0;
    float gps_alt = f32le(p + 60);
    uint8_t gps_fix = p[64];
    uint8_t gps_sats = p[65];
    // sat_view 필드: 66-72 (7 bytes)
    // Offset 67=total, 68=gps, 69=glo, 70=gal, 71=bei
    uint8_t sats_gps = p[68];
    uint8_t sats_gl = p[69];
    uint8_t sats_ga = p[70];
    uint8_t sats_gb = p[71];
    
    // GPS UTC 시간 파싱 (offset 73-79: hour, min, sec, day, month, year[2])
    uint8_t utc_hour = p[73];
    uint8_t utc_min = p[74];
    uint8_t utc_sec = p[75];
    uint8_t utc_day = p[76];
    uint8_t utc_month = p[77];
    uint16_t utc_year = u16le(p + 78);

    // KST (UTC+9)로 변환
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
    
    // 배터리 전압 (offset 80)
    uint16_t bat_mv = u16le(p + 80);
    
    // 대기질 (offset 82-89)
    uint16_t pm1 = u16le(p + 82);
    uint16_t pm25 = u16le(p + 84);
    uint16_t pm10 = u16le(p + 86);
    int16_t ozone = i16le(p + 88);
    
    // 기압 / 습도 (offset 90-97)
    float humidity = u16le(p + 90) / 100.0f;
    uint32_t press_pa = u32le(p + 92);
    float press_temp = i16le(p + 96) / 100.0f;
    
    // 방사선 (offset 98)
    float radiation = u16le(p + 98) / 100.0f;
    
    // 히터 (offset 100-101)
    uint8_t heater_bat = p[100];
    uint8_t heater_board = p[101];
    
    // 고도 융합 (offset 102-117, 이미 float)
    float press_alt = f32le(p + 102);
    float kf_alt = f32le(p + 106);
    float kf_roll = f32le(p + 110);
    float kf_pitch = f32le(p + 114);
    
    // GPS 시간을 포함한 실제 값으로 CSV 라인 작성
    logFile.printf("%lu,%u,%lu,0x%04X,",
                   millis(), seq, ts_ms, status);
    // GPS 일시 (ISO 8601 포맷: YYYY-MM-DDTHH:MM:SS)
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

// ===== LoRa 함수 =====

/** 
 * @brief LoRa 패킷 전송 (UART 원본 데이터)
 * @details 수신된 바이너리 프레임을 그대로 공중으로 전송합니다. LBT(Listen Before Talk)를 수행합니다.
 */
void sendLoRaPacket() {
  if (latestFrameLen == 0) return;
  
  // CSMA / LBT (Listen Before Talk) - 채널 점유 확인
  // 현재 수신 중인 패킷이 있는지 확인
  // parsePacket()은 읽기 대기 중인 패킷 크기를 반환
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
      Serial.printf("[LoRa] Channel Busy (RX %d bytes), Skipping TX to avoid collision\n", packetSize);
      // 충돌 회피를 위해 전송 건너뜀 (다음 사이클에 재시도 가능)
      return;
  }
  
  // 채널이 비어있으면 전송 진행
  LoRa.beginPacket();
  LoRa.write(latestFrame, latestFrameLen);
  LoRa.endPacket();

  // 즉시 수신 모드로 복귀
  LoRa.receive();

  loraTxCount++;
  
  uint16_t seq = u16le(&latestFrame[6]);
  Serial.printf("[LoRa] TX raw seq=%u len=%u bytes\n", seq, latestFrameLen);
}

// ===== 프레임 처리 함수 =====

/** 
 * @brief 수신된 UART 프레임 처리
 * @details CRC 검증 후 SD 버퍼에 저장하고, LoRa 전송용 최신 프레임을 갱신합니다. 또한 GPS 시간을 추출하여 시스템 시간을 동기화합니다.
 * @param frame 수신된 프레임 데이터 포인터
 * @param len 프레임 길이
 */
void processFrame(uint8_t *frame, size_t len) {
  const uint16_t payload_len = u16le(&frame[4]);
  const uint16_t seq = u16le(&frame[6]);
  const uint32_t ts_ms = u32le(&frame[8]);
  
  // CRC 무결성 검증
  const uint16_t crc_rx = u16le(&frame[12 + payload_len]);
  const uint16_t crc_calc = crc16_ccitt_false(frame, 12 + payload_len);
  
  if (crc_rx != crc_calc) {
    crcErrors++;
    Serial.printf("[RX] CRC ERROR seq=%u\n", seq);
    return;
  }
  
  rxCount++;
  
  // SD 저장용 버퍼에 추가
  if (bufferCount < FRAME_BUFFER_SIZE) {
    memcpy(frameBuffer[bufferCount], frame, len);
    frameLengths[bufferCount] = len;
    bufferCount++;
  }
  
  // LoRa 전송용 최신 프레임 갱신 (Raw Binary)
  memcpy(latestFrame, frame, len);
  latestFrameLen = len;
  
  // SD 타임스탬프를 위한 전역 GPS 시간 업데이트
  // 프레임 구조: [Header 12] [Payload...] [CRC 2]
  // Payload 시작 오프셋 = 12
  // Payload 내 날짜/시간 오프셋:
  // Hour: 73, Min: 74, Sec: 75, Day: 76, Month: 77, Year: 78 (uint16)
  
  const uint8_t *p = &frame[12];
  uint16_t year = u16le(p + 78);
  
  // 기본 유효성 검사 (Year > 2020)
  if (year > 2020) {
      // SD 타임스탬프용 UTC -> KST(UTC+9) 변환
      // 1. UTC 값 가져오기
      uint16_t t_year = year;
      uint8_t t_month = p[77];
      uint8_t t_day = p[76];
      uint8_t t_hour = p[73];
      uint8_t t_min = p[74];
      uint8_t t_sec = p[75];

      // 2. 9시간 더하기
      t_hour += 9;
      
      // 3. 날짜 변경(Rollover) 처리
      if (t_hour >= 24) {
        t_hour -= 24;
        t_day++;
        
        // 월별 일수 계산
        uint8_t daysInMonth = 31;
        if (t_month == 4 || t_month == 6 || t_month == 9 || t_month == 11) {
          daysInMonth = 30;
        } else if (t_month == 2) {
          // 윤년 확인
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

      // 4. 전역 변수 업데이트
      gpsYear = t_year;
      gpsMonth = t_month;
      gpsDay = t_day;
      gpsHour = t_hour;
      gpsMin = t_min;
      gpsSec = t_sec;
      
      // 5. SD 파일 타임스탬프를 위한 시스템 클럭 동기화
      syncInternalClock(gpsYear, gpsMonth, gpsDay, gpsHour, gpsMin, gpsSec);
  }
  
  // 디버그 출력 (매 50프레임 = 약 1초마다)
  if (rxCount % 50 == 0) {
    Serial.printf("[RX] seq=%u ts=%lu status=0x%04X buf=%d\n", 
                  seq, ts_ms, u16le(&frame[16]), bufferCount);
  }
}

/**
 * @brief 초기화 함수 (Setup)
 * @details 시리얼 통신, SD 카드, LoRa 모듈을 초기화하고 타임스탬프를 리셋합니다.
 */
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== LoRa32 v2.1 Telemetry Receiver ===");
  
  // UART from STM32 (버퍼 확장: SD 쓰기 중 데이터 손실 방지)
  Serial2.setRxBufferSize(1024);  // 8프레임(160ms) 분량
  Serial2.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  Serial.printf("[UART] RX=%d TX=%d\n", STM32_RX_PIN, STM32_TX_PIN);
  
  // SD 카드 초기화
  if (SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] Initialized");
    createNewLogFile();
  } else {
    Serial.println("[SD] Init FAILED!");
  }
  
  // LoRa 초기화 (LoRa32 v2.1 고정 SPI 핀 사용)
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

/**
 * @brief 메인 루프 (Loop)
 * @details UART 수신, SD 카드 기록, LoRa 전송을 주기적으로 수행합니다.
 */
void loop() {
  uint32_t now = millis();
  
  // UART 데이터 읽기
  while (Serial2.available() > 0) {
    const uint8_t b = (uint8_t)Serial2.read();
    
    // 매직 바이트 동기화
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
    
    // 헤더 수신 완료 - 예상 길이 파악
    if (rxIdx == 12) {
      const uint16_t payload_len = u16le(&rxFrame[4]);
      rxNeed = 12 + (size_t)payload_len + 2;  // + CRC16
      if (rxNeed > sizeof(rxFrame)) {
        resetParser();
      }
      continue;
    }
    
    // 프레임 수신 완료
    if (rxNeed && rxIdx == rxNeed) {
      processFrame(rxFrame, rxIdx);
      resetParser();
    }
    
    // 버퍼 오버플로우 방지
    if (rxIdx >= sizeof(rxFrame)) {
      resetParser();
    }
  }
  
  // SD 카드 기록 (1초 주기)
  if (now - lastSdWrite >= SD_WRITE_INTERVAL_MS) {
    writeBufferToSd();
    lastSdWrite = now;
  }
  
  // LoRa 전송 (5초 주기) - UART 원본 바이너리 전송
  if (now - lastLoraTx >= LORA_TX_INTERVAL_MS) {
    sendLoRaPacket();
    lastLoraTx = now;
  }
}
