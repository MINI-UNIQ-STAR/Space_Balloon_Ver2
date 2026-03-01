#include "lora.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define REG_FIFO 0x00
#define REG_OP_MODE 0x01
#define REG_FRF_MSB 0x06
#define REG_FRF_MID 0x07
#define REG_FRF_LSB 0x08
#define REG_PA_CONFIG 0x09
#define REG_FIFO_ADDR_PTR 0x0d
#define REG_FIFO_TX_BASE_ADDR 0x0e
#define REG_FIFO_RX_BASE_ADDR 0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS 0x12
#define REG_RX_NB_BYTES 0x13
#define REG_PKT_SNR_VALUE 0x19
#define REG_PKT_RSSI_VALUE 0x1a
#define REG_MODEM_CONFIG_1 0x1d
#define REG_MODEM_CONFIG_2 0x1e
#define REG_MODEM_CONFIG_3 0x26
#define REG_PAYLOAD_LENGTH 0x22
#define REG_HOP_PERIOD 0x24
#define REG_FIFO_RX_BYTE_ADDR 0x25
#define REG_DIO_MAPPING_1 0x40
#define REG_VERSION 0x42

// modes
#define MODE_LONG_RANGE_MODE 0x80
#define MODE_SLEEP 0x00
#define MODE_STDBY 0x01
#define MODE_TX 0x03
#define MODE_RX_CONTINUOUS 0x05
#define MODE_RX_SINGLE 0x06

// PA config
#define PA_BOOST 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK 0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK 0x40

#define MAX_PKT_LENGTH 255

static void set_ldo_flag(void);

static spi_device_handle_t __spi;
static int __cs;
static int __rst;
static int __ss;
static long __frequency;
static int __packetIndex;

static const char *TAG = "LORA";

static uint8_t read_reg(uint8_t reg) {
  uint8_t rx_data;
  spi_transaction_t t = {.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
                         .length = 16,
                         .tx_data = {reg & 0x7F, 0x00}};
  gpio_set_level(__cs, 0);
  spi_device_transmit(__spi, &t);
  gpio_set_level(__cs, 1);
  return t.rx_data[1];
}

static void write_reg(uint8_t reg, uint8_t val) {
  spi_transaction_t t = {.flags = SPI_TRANS_USE_TXDATA,
                         .length = 16,
                         .tx_data = {reg | 0x80, val}};
  gpio_set_level(__cs, 0);
  spi_device_transmit(__spi, &t);
  gpio_set_level(__cs, 1);
}

void lora_sleep(void) {
  write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}
void lora_idle(void) {
  write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}
void lora_receive(void) {
  write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

void lora_set_frequency(long frequency) {
  __frequency = frequency;
  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
  write_reg(REG_FRF_MSB, (uint8_t)(frf >> 16));
  write_reg(REG_FRF_MID, (uint8_t)(frf >> 8));
  write_reg(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

void lora_set_tx_power(int level) {
  if (level < 2)
    level = 2;
  else if (level > 17)
    level = 17;
  write_reg(REG_PA_CONFIG, PA_BOOST | (level - 2));
}

void lora_set_spreading_factor(int sf) {
  if (sf < 6)
    sf = 6;
  else if (sf > 12)
    sf = 12;
  if (sf == 6) {
    write_reg(0x31, 0xc5);
    write_reg(0x37, 0x0c);
  } else {
    write_reg(0x31, 0xc3);
    write_reg(0x37, 0x0a);
  }
  write_reg(REG_MODEM_CONFIG_2,
            (read_reg(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
  set_ldo_flag();
}

static void set_ldo_flag(void) {
  long bw = 125000;
  uint8_t config1 = read_reg(REG_MODEM_CONFIG_1);
  uint8_t bw_reg = config1 >> 4;
  switch (bw_reg) {
  case 7:
    bw = 125000;
    break;
  case 8:
    bw = 250000;
    break;
  case 9:
    bw = 500000;
    break;
  }
  uint8_t config2 = read_reg(REG_MODEM_CONFIG_2);
  int sf = config2 >> 4;
  long symbolDuration = 1000 / (bw / (1L << sf));
  bool ldoOn = symbolDuration > 16;
  uint8_t config3 = read_reg(REG_MODEM_CONFIG_3);
  if (ldoOn)
    config3 |= 1 << 3;
  else
    config3 &= ~(1 << 3);
  write_reg(REG_MODEM_CONFIG_3, config3);
}

void lora_set_signal_bandwidth(long sbw) {
  int bw;
  if (sbw <= 7.8E3)
    bw = 0;
  else if (sbw <= 10.4E3)
    bw = 1;
  else if (sbw <= 15.6E3)
    bw = 2;
  else if (sbw <= 20.8E3)
    bw = 3;
  else if (sbw <= 31.25E3)
    bw = 4;
  else if (sbw <= 41.7E3)
    bw = 5;
  else if (sbw <= 62.5E3)
    bw = 6;
  else if (sbw <= 125E3)
    bw = 7;
  else if (sbw <= 250E3)
    bw = 8;
  else
    bw = 9;
  write_reg(REG_MODEM_CONFIG_1,
            (read_reg(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
  set_ldo_flag();
}

void lora_set_coding_rate4(int denominator) {
  if (denominator < 5)
    denominator = 5;
  else if (denominator > 8)
    denominator = 8;
  int cr = denominator - 4;
  write_reg(REG_MODEM_CONFIG_1,
            (read_reg(REG_MODEM_CONFIG_1) & 0xf1) | (cr << 1));
}

int lora_init(spi_host_device_t spi_host, int rst_pin, int cs_pin, int d0_pin,
              long frequency) {
  __rst = rst_pin;
  __cs = cs_pin;
  gpio_reset_pin(__cs);
  gpio_set_direction(__cs, GPIO_MODE_OUTPUT);
  gpio_set_level(__cs, 1);
  if (__rst != -1) {
    gpio_reset_pin(__rst);
    gpio_set_direction(__rst, GPIO_MODE_OUTPUT);
    gpio_set_level(__rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(__rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = 8 * 1000 * 1000,
      .mode = 0,
      .spics_io_num = -1,
      .queue_size = 7,
  };
  if (spi_bus_add_device(spi_host, &devcfg, &__spi) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add SPI device");
    return -1;
  }

  uint8_t version = read_reg(REG_VERSION);
  if (version != 0x12) {
    ESP_LOGE(TAG, "Unrecognized transceiver version (0x%x)", version);
    return -1;
  }

  lora_sleep();
  lora_set_frequency(frequency);
  write_reg(REG_FIFO_TX_BASE_ADDR, 0);
  write_reg(REG_FIFO_RX_BASE_ADDR, 0);
  write_reg(0x0C, read_reg(0x0C) | 0x03);
  write_reg(REG_MODEM_CONFIG_3, 0x04);
  lora_set_tx_power(17);
  lora_idle();
  return 0;
}

int lora_init_bus_and_device(spi_host_device_t spi_host, int miso, int mosi,
                             int sck, int rst_pin, int cs_pin, int d0_pin,
                             long frequency) {
  spi_bus_config_t buscfg = {
      .miso_io_num = miso,
      .mosi_io_num = mosi,
      .sclk_io_num = sck,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  if (spi_bus_initialize(spi_host, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPI bus");
    return -1;
  }
  return lora_init(spi_host, rst_pin, cs_pin, d0_pin, frequency);
}

int lora_begin_packet(int implicitHeader) {
  if (implicitHeader) {
    write_reg(REG_MODEM_CONFIG_1, read_reg(REG_MODEM_CONFIG_1) | 0x01);
  } else {
    write_reg(REG_MODEM_CONFIG_1, read_reg(REG_MODEM_CONFIG_1) & 0xfe);
  }
  lora_idle();
  write_reg(REG_FIFO_ADDR_PTR, 0);
  write_reg(REG_PAYLOAD_LENGTH, 0);
  return 1;
}

int lora_end_packet(void) {
  write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);
  while ((read_reg(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
    vTaskDelay(1);
  }
  write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
  return 1;
}

size_t lora_write(const uint8_t *buffer, size_t size) {
  int currentLength = read_reg(REG_PAYLOAD_LENGTH);
  if ((currentLength + size) > MAX_PKT_LENGTH)
    size = MAX_PKT_LENGTH - currentLength;
  for (size_t i = 0; i < size; i++) {
    write_reg(REG_FIFO, buffer[i]);
  }
  write_reg(REG_PAYLOAD_LENGTH, currentLength + size);
  return size;
}

int lora_parse_packet(void) {
  int packetLength = 0;
  int irqFlags = read_reg(REG_IRQ_FLAGS);
  if (1) {
    lora_idle();
    if ((irqFlags & IRQ_RX_DONE_MASK) &&
        (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
      __packetIndex = 0;
      packetLength = read_reg(REG_RX_NB_BYTES);
      write_reg(REG_FIFO_ADDR_PTR, read_reg(REG_FIFO_RX_CURRENT_ADDR));
      lora_idle();
    } else if (read_reg(REG_OP_MODE) !=
               (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) {
      write_reg(REG_FIFO_ADDR_PTR, 0);
      lora_receive();
    }
  }
  write_reg(REG_IRQ_FLAGS, irqFlags);
  return packetLength;
}

size_t lora_read(uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; i++) {
    buffer[i] = read_reg(REG_FIFO);
  }
  return size;
}
