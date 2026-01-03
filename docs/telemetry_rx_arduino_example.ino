// telemetry_rx_arduino_example.ino
//
// ESP32 Arduino: receive STM32 telemetry frames over UART (Serial2)
// and print a CSV log over USB Serial for debugging.
//
// Frame format matches Core/Inc/services/telemetry_frame.h:
// [0..1]  magic = 0xA5 0x5A
// [2]     version
// [3]     msg_type
// [4..5]  payload_len (LE)
// [6..7]  seq (LE)
// [8..11] timestamp_ms (LE, STM32 HAL_GetTick())
// [12..]  payload bytes
// [..]    crc16_ccitt_false(header+payload), appended LE (lo, hi)
//
// Logs (CSV): rx_t_us,seq,crc_ok,frame_ts_ms,msg_type,payload_len
//
// Notes:
// - This example intentionally does NOT parse the payload struct.
//   (Payload layout may evolve; header+CRC is enough for 50Hz jitter/drop debugging.)
// - Use a separate UART for STM32 RX (Serial2) and keep Serial for PC logging.

#include <Arduino.h>

static uint16_t u16le(const uint8_t *p) { return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }
static uint32_t u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
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

static uint8_t frame[1024];
static size_t idx = 0;
static size_t need = 0;

static void reset_parser() {
  idx = 0;
  need = 0;
}

void setup() {
  Serial.begin(115200);   // USB Serial to PC
  Serial2.begin(115200);  // UART from STM32 (adjust pins/baud to your wiring)

  // CSV header
  Serial.println("rx_t_us,seq,crc_ok,frame_ts_ms,msg_type,payload_len");
}

void loop() {
  while (Serial2.available() > 0) {
    const uint8_t b = (uint8_t)Serial2.read();

    // Sync on magic bytes
    if (idx == 0) {
      if (b == 0xA5) frame[idx++] = b;
      continue;
    }
    if (idx == 1) {
      if (b == 0x5A) frame[idx++] = b;
      else reset_parser();
      continue;
    }

    frame[idx++] = b;

    // Once we have the fixed 12-byte header, compute expected frame length
    if (idx == 12) {
      const uint16_t payload_len = u16le(&frame[4]);
      need = 12 + (size_t)payload_len + 2;  // + CRC16
      if (need > sizeof(frame)) {
        reset_parser();
      }
      continue;
    }

    // Frame complete
    if (need && idx == need) {
      const uint16_t payload_len = u16le(&frame[4]);
      const uint16_t seq = u16le(&frame[6]);
      const uint32_t ts_ms = u32le(&frame[8]);
      const uint8_t msg_type = frame[3];

      const uint16_t crc_rx = u16le(&frame[12 + payload_len]);
      const uint16_t crc_calc = crc16_ccitt_false(frame, 12 + payload_len);
      const int crc_ok = (crc_rx == crc_calc) ? 1 : 0;

      const uint32_t rx_t_us = micros();

      Serial.printf("%lu,%u,%d,%lu,%u,%u\n",
                    (unsigned long)rx_t_us,
                    (unsigned)seq,
                    crc_ok,
                    (unsigned long)ts_ms,
                    (unsigned)msg_type,
                    (unsigned)payload_len);

      reset_parser();
    }

    // Safety: if buffer fills without finding a valid length, resync
    if (idx >= sizeof(frame)) {
      reset_parser();
    }
  }
}
