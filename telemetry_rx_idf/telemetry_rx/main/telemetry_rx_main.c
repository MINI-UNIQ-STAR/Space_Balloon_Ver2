#include "lora.h"
#include "ssd1306.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_vfs_fat.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <sdmmc_cmd.h>
#include <stdio.h>
#include <string.h>

// ========== PIN CONFIGURATION ==========
// STM32 UART
#define UART_PORT_NUM UART_NUM_1
#define PIN_UART_TX 12
#define PIN_UART_RX 13
#define UART_BAUD_RATE 115200

// OLED I2C
#define I2C_PORT_NUM I2C_NUM_0
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// LoRa SPI (VSPI)
#define LORA_SPI_HOST SPI3_HOST
#define PIN_LORA_SCK 5
#define PIN_LORA_MISO 19
#define PIN_LORA_MOSI 27
#define PIN_LORA_CS 18
#define PIN_LORA_RST 23
#define PIN_LORA_DIO0 26

// SD SPI (HSPI)
#define SD_SPI_HOST SPI2_HOST
#define PIN_SD_SCK 14
#define PIN_SD_MISO 2
#define PIN_SD_MOSI 15
#define PIN_SD_CS 13

#define LORA_FREQ 915E6

static const char *TAG = "MAIN";

// ========== DATA STRUCTURES ==========
#pragma pack(push, 1)
typedef struct {
  uint32_t uptime_ms;
  uint16_t status_flags;
  uint16_t co2_ppm;
  int32_t accel_mps2_x1000[3];
  int32_t gyro_rads_x1000[3];
  float mag_uT[3];
  int16_t board_temp_c_x100;
  int16_t external_temp_c_x100;
  int16_t sht31_temp_c_x100;
  int16_t bat_temp_c_x100;
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
  uint8_t gps_utc_hour;
  uint8_t gps_utc_min;
  uint8_t gps_utc_sec;
  uint8_t gps_utc_day;
  uint8_t gps_utc_month;
  uint16_t gps_utc_year;
  uint16_t bat_mv;
  uint16_t pm1_ugm3;
  uint16_t pm25_ugm3;
  uint16_t pm10_ugm3;
  int16_t ozone_ppb;
  uint16_t sht31_rh_x100;
  uint32_t ms5611_press_pa;
  int16_t ms5611_temp_c_x100;
  uint16_t gdk101_usvh_x100;
  uint8_t heater_bat_duty_percent;
  uint8_t heater_board_duty_percent;
  float press_alt_m;
  float kf_alt_m;
  float kf_roll_deg;
  float kf_pitch_deg;
} telemetry_payload_t;

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

// ========== GLOBAL QUEUES & STATE ==========
static QueueHandle_t sd_queue;
static QueueHandle_t lora_queue;

typedef struct {
  uint32_t rx_count;
  uint32_t crc_errors;
  uint32_t sd_writes;
  uint32_t lora_tx;
  float alt;
  uint8_t sats;
  bool sd_ok;
  bool lora_ok;
} app_state_t;

static app_state_t state = {0};

static uint16_t crc16_ccitt_false(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; b++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                           : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

// ========== TASKS ==========

// OLED Display Task
static void oled_task(void *pvParameters) {
  ssd1306_init(I2C_PORT_NUM, PIN_I2C_SDA, PIN_I2C_SCL);
  ssd1306_clear();

  char buf[32];
  while (1) {
    ssd1306_clear();

    snprintf(buf, sizeof(buf), "RX: %lu CRC: %lu", state.rx_count,
             state.crc_errors);
    ssd1306_print(0, 0, buf);

    snprintf(buf, sizeof(buf), "SD: %lu %s", state.sd_writes,
             state.sd_ok ? "OK" : "ERR");
    ssd1306_print(1, 0, buf);

    snprintf(buf, sizeof(buf), "LoRa: %lu", state.lora_tx);
    ssd1306_print(2, 0, buf);

    snprintf(buf, sizeof(buf), "Alt: %.1fm", state.alt);
    ssd1306_print(3, 0, buf);

    snprintf(buf, sizeof(buf), "Sats: %u", state.sats);
    ssd1306_print(4, 0, buf);

    ssd1306_show();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// LoRa TX Task
static void lora_task(void *pvParameters) {
  if (lora_init_bus_and_device(LORA_SPI_HOST, PIN_LORA_MISO, PIN_LORA_MOSI,
                               PIN_LORA_SCK, PIN_LORA_RST, PIN_LORA_CS,
                               PIN_LORA_DIO0, LORA_FREQ) == 0) {
    ESP_LOGI(TAG, "LoRa Init OK");
    lora_set_spreading_factor(11);
    lora_set_signal_bandwidth(125E3);
    lora_set_coding_rate4(5);
    state.lora_ok = true;
  } else {
    ESP_LOGE(TAG, "LoRa Init FAIL");
    state.lora_ok = false;
  }

  telemetry_frame_t tx_frame;
  TickType_t last_wake_time = xTaskGetTickCount();

  while (1) {
    // Wait exactly 5 seconds
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(5000));

    // Get latest frame from queue (drain queue to just get the most recent)
    bool has_frame = false;
    while (xQueueReceive(lora_queue, &tx_frame, 0) == pdTRUE) {
      has_frame = true;
    }

    if (has_frame && state.lora_ok) {
      ESP_LOGI(TAG, "LoRa TX seq=%u", tx_frame.seq);
      lora_begin_packet(0); // explicit header
      lora_write((uint8_t *)&tx_frame, sizeof(telemetry_frame_t));
      lora_end_packet();
      state.lora_tx++;
    }
  }
}

// SD Log Task
static void sd_task(void *pvParameters) {
  // Mount SD Card
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};
  sdmmc_card_t *card;
  const char mount_point[] = "/sdcard";

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SD_SPI_HOST;

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = PIN_SD_MOSI,
      .miso_io_num = PIN_SD_MISO,
      .sclk_io_num = PIN_SD_SCK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };

  esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SD SPI bus.");
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_SD_CS;
  slot_config.host_id = host.slot;

  ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config,
                                &card);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "SD Card Mounted.");
    state.sd_ok = true;
  } else {
    ESP_LOGE(TAG, "Failed to mount SD card.");
    state.sd_ok = false;
  }

  telemetry_frame_t f;
  TickType_t last_wake_time = xTaskGetTickCount();

  while (1) {
    // Log every 1 second
    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));

    bool has_frame = false;
    // drain queue to get latest
    while (xQueueReceive(sd_queue, &f, 0) == pdTRUE) {
      has_frame = true;
    }

    if (has_frame && state.sd_ok) {
      // Write to SD as CSV
      FILE *file = fopen("/sdcard/telem.csv", "a");
      if (file != NULL) {
        // Determine KST
        uint16_t yr = f.payload.gps_utc_year;
        uint8_t mo = f.payload.gps_utc_month;
        uint8_t dy = f.payload.gps_utc_day;
        uint8_t hr = f.payload.gps_utc_hour;
        uint8_t mn = f.payload.gps_utc_min;
        uint8_t sc = f.payload.gps_utc_sec;
        if (yr > 2000)
          hr += 9;
        if (hr >= 24) {
          hr -= 24;
          dy++;
        } // rough day rollover

        fprintf(file, "%lld,%u,%lu,0x%04X,", esp_timer_get_time() / 1000, f.seq,
                f.timestamp_ms, f.payload.status_flags);
        fprintf(file, "%04u-%02u-%02uT%02u:%02u:%02u,", yr, mo, dy, hr, mn, sc);
        fprintf(file, "%.7f,%.7f,%.2f,%u,%u,", f.payload.gps_lat_deg_e7 / 1e7,
                f.payload.gps_lon_deg_e7 / 1e7, f.payload.gps_alt_m,
                f.payload.gps_fix, f.payload.gps_sats_used);
        fprintf(file, "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,",
                f.payload.accel_mps2_x1000[0] / 1000.0,
                f.payload.accel_mps2_x1000[1] / 1000.0,
                f.payload.accel_mps2_x1000[2] / 1000.0,
                f.payload.gyro_rads_x1000[0] / 1000.0,
                f.payload.gyro_rads_x1000[1] / 1000.0,
                f.payload.gyro_rads_x1000[2] / 1000.0);
        fprintf(file, "%.2f,%.2f,%.2f,", f.payload.mag_uT[0],
                f.payload.mag_uT[1], f.payload.mag_uT[2]);
        fprintf(file, "%.2f,%.2f,%.2f,%.2f,",
                f.payload.board_temp_c_x100 / 100.0,
                f.payload.external_temp_c_x100 / 100.0,
                f.payload.sht31_temp_c_x100 / 100.0,
                f.payload.bat_temp_c_x100 / 100.0);
        fprintf(file, "%lu,%.2f,%.2f,", f.payload.ms5611_press_pa,
                f.payload.ms5611_temp_c_x100 / 100.0,
                f.payload.sht31_rh_x100 / 100.0);
        fprintf(file, "%u,%u,%u,%u,%d,%.2f,", f.payload.co2_ppm,
                f.payload.pm1_ugm3, f.payload.pm25_ugm3, f.payload.pm10_ugm3,
                f.payload.ozone_ppb, f.payload.gdk101_usvh_x100 / 100.0);
        fprintf(file, "%u,%u,%u,", f.payload.bat_mv,
                f.payload.heater_bat_duty_percent,
                f.payload.heater_board_duty_percent);
        fprintf(file, "%.2f,%.2f,%.2f,%.2f\n", f.payload.press_alt_m,
                f.payload.kf_alt_m, f.payload.kf_roll_deg,
                f.payload.kf_pitch_deg);
        fclose(file);
        state.sd_writes++;
      }
    }
  }
}

// UART RX Task
static void uart_task(void *pvParameters) {
  uart_config_t uart_config = {
      .baud_rate = UART_BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  int intr_alloc_flags = 0;
  uart_driver_install(UART_PORT_NUM, 1024, 0, 0, NULL, intr_alloc_flags);
  uart_param_config(UART_PORT_NUM, &uart_config);
  uart_set_pin(UART_PORT_NUM, PIN_UART_TX, PIN_UART_RX, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);

  uint8_t rx_buffer[256];
  size_t rx_idx = 0;

  while (1) {
    uint8_t b;
    int len = uart_read_bytes(UART_PORT_NUM, &b, 1, portMAX_DELAY);
    if (len > 0) {
      if (rx_idx == 0 && b != 0xA5)
        continue;
      if (rx_idx == 1 && b != 0x5A) {
        rx_idx = 0;
        continue;
      }

      rx_buffer[rx_idx++] = b;

      if (rx_idx >= 12) {
        uint16_t payload_len = rx_buffer[4] | (rx_buffer[5] << 8);
        size_t expected_len = 12 + payload_len + 2; // + CRC

        if (rx_idx == expected_len) {
          if (expected_len == sizeof(telemetry_frame_t)) {
            telemetry_frame_t *frame = (telemetry_frame_t *)rx_buffer;
            uint16_t calc_crc = crc16_ccitt_false(rx_buffer, 12 + payload_len);
            if (calc_crc == frame->crc16) {
              ESP_LOGI(TAG, "UART RX: seq=%u, crc=OK, alt=%.1fm", frame->seq,
                       frame->payload.kf_alt_m);
              state.rx_count++;
              state.alt = frame->payload.kf_alt_m;
              state.sats = frame->payload.gps_sats_used;

              // Send to queues
              xQueueSend(sd_queue, frame, 0);
              xQueueSend(lora_queue, frame, 0);
            } else {
              state.crc_errors++;
            }
          }
          rx_idx = 0;
        } else if (expected_len > sizeof(rx_buffer)) {
          rx_idx = 0;
        }
      }
    }
  }
}

void app_main(void) {
  ESP_LOGI(TAG, "Starting TTGO LoRa32 v2.1 Telemetry Receiver");

  sd_queue = xQueueCreate(10, sizeof(telemetry_frame_t));
  lora_queue = xQueueCreate(5, sizeof(telemetry_frame_t));

  xTaskCreatePinnedToCore(oled_task, "oled_task", 4096, NULL, 5, NULL, 0);
  xTaskCreatePinnedToCore(sd_task, "sd_task", 8192, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(lora_task, "lora_task", 8192, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(uart_task, "uart_task", 4096, NULL, 10, NULL, 0);
}
