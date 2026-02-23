/**
 * @file telemetry_zephyr.c
 * @brief 텔레메트리 - Zephyr RTOS 포팅 (UART3 148바이트 프레임)
 */

#include "telemetry_zephyr.h"
#include "fdir_zephyr.h"
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(telemetry, LOG_LEVEL_INF);

/* ========================================================================== */
/* 프레임 구조 정의 */
/* ========================================================================== */

#define TELEMETRY_MAGIC_0 0xA5
#define TELEMETRY_MAGIC_1 0x5A
#define TELEMETRY_VERSION 1
#define TELEMETRY_MSG_TYPE 0x02 /* Sensor Snapshot */
#define TELEMETRY_PAYLOAD_LEN sizeof(telemetry_payload_t)
#define TELEMETRY_FRAME_SIZE sizeof(telemetry_frame_t)

#pragma pack(push, 1)

/* telemetry_payload_t is now in telemetry_zephyr.h */

typedef struct {
  uint8_t magic[2];
  uint8_t version;
  uint8_t msg_type;
  uint16_t payload_len;
  uint16_t seq;
  uint32_t timestamp_ms;
  telemetry_payload_t payload;
  uint16_t crc16;
} telemetry_frame_t;

#pragma pack(pop)

/* ========================================================================== */
/* 전역 변수 */
/* ========================================================================== */

static uint16_t seq_number = 0;
static telemetry_frame_t tx_frame;

/* ========================================================================== */
/* UART 장치 (실제 하드웨어용) */
/* ========================================================================== */

#if defined(CONFIG_BOARD_WEARCT_STM32G431_CORE) ||                             \
    defined(CONFIG_BOARD_NUCLEO_G431RB)
#define TELEMETRY_UART_ENABLED 1
#define UART_DEV_NODE DT_NODELABEL(usart3)
#elif defined(CONFIG_BOARD_QEMU_CORTEX_M3)
/* QEMU 시뮬레이션: uart1 (두 번째 직렬 포트) 사용 */
#define TELEMETRY_UART_ENABLED 1
#define UART_DEV_NODE DT_NODELABEL(uart1)
#else
/* 다른 QEMU/보드 시뮬레이션 */
#define TELEMETRY_UART_ENABLED 0
#define UART_DEV_NODE DT_INVALID_NODE
#endif

static const struct device *uart_dev = NULL;
static bool uart_ready = false;

/* ========================================================================== */
/* CRC-16/CCITT-FALSE                                                         */
/* ========================================================================== */

static uint16_t crc16_ccitt(const uint8_t *data, uint16_t length) {
  uint16_t crc = 0xFFFF;

  for (uint16_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc = crc << 1;
      }
    }
  }

  return crc;
}

/* ========================================================================== */
/* 초기화 */
/* ========================================================================== */

void Telemetry_Init(void) {
#if TELEMETRY_UART_ENABLED
  uart_dev = DEVICE_DT_GET(UART_DEV_NODE);
  if (device_is_ready(uart_dev)) {
    uart_ready = true;
    LOG_INF("Telemetry initialized (UART3, 148 bytes/frame, BINARY)");
  } else {
    LOG_ERR("UART3 not ready");
  }
#else
  LOG_INF("Telemetry initialized (QEMU Simulation, log only)");
#endif
}

/* ========================================================================== */
/* 프레임 생성 */
/* ========================================================================== */

void Telemetry_BuildFrame(telemetry_payload_t *payload) {
  /* 헤더 */
  tx_frame.magic[0] = TELEMETRY_MAGIC_0;
  tx_frame.magic[1] = TELEMETRY_MAGIC_1;
  tx_frame.version = TELEMETRY_VERSION;
  tx_frame.msg_type = TELEMETRY_MSG_TYPE;
  tx_frame.payload_len = TELEMETRY_PAYLOAD_LEN;
  tx_frame.seq = seq_number++;
  tx_frame.timestamp_ms = payload->uptime_ms;

  /* 페이로드 복사 */
  memcpy(&tx_frame.payload, payload, sizeof(telemetry_payload_t));

  /* CRC 계산 (헤더 + 페이로드) */
  tx_frame.crc16 =
      crc16_ccitt((const uint8_t *)&tx_frame, sizeof(telemetry_frame_t) - 2);
}

/* ========================================================================== */
/* 프레임 전송 */
/* ========================================================================== */

void Telemetry_Send(void) {
#if TELEMETRY_UART_ENABLED
  /* 실제 하드웨어: UART3로 148바이트 바이너리 전송 */
  if (uart_ready && uart_dev != NULL) {
    /* UART 폴링 전송 */
    for (int i = 0; i < TELEMETRY_FRAME_SIZE; i++) {
      uart_poll_out(uart_dev, ((uint8_t *)&tx_frame)[i]);
    }

    /* 디버그 로그 (1초마다) */
    if (tx_frame.seq % 50 == 0) {
      LOG_INF("TX: seq=%u, flags=0x%04X, crc=0x%04X", tx_frame.seq,
              tx_frame.payload.status_flags, tx_frame.crc16);
    }
  }
#else
  /* QEMU 시뮬레이션: 로그만 출력 */
  LOG_INF("TELEMETRY FRAME: seq=%u, uptime=%ums, flags=0x%04X, press=%uPa, "
          "alt=%.1fm",
          tx_frame.seq, tx_frame.timestamp_ms, tx_frame.payload.status_flags,
          tx_frame.payload.ms5611_press_pa, tx_frame.payload.press_alt_m);
#endif
}

/* ========================================================================== */
/* 전체 텔레메트리 처리 */
/* ========================================================================== */

void Telemetry_Process(telemetry_payload_t *payload) {
  Telemetry_BuildFrame(payload);
  Telemetry_Send();
}
