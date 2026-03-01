#ifndef __LORA_H__
#define __LORA_H__

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the LoRa SX1276 module.
 * @param spi_host SPI host (e.g., VSPI_HOST)
 * @param rst_pin Reset pin
 * @param cs_pin Chip select pin
 * @param d0_pin DIO0 pin for interrupts
 * @param frequency Frequency in Hz
 * @return 0 on success, -1 on failure
 */
int lora_init(spi_host_device_t spi_host, int rst_pin, int cs_pin, int d0_pin,
              long frequency);

/**
 * @brief Initialize a LoRa SPI bus and then call lora_init (Convenience
 * function).
 */
int lora_init_bus_and_device(spi_host_device_t spi_host, int miso, int mosi,
                             int sck, int rst_pin, int cs_pin, int d0_pin,
                             long frequency);

void lora_set_frequency(long frequency);
void lora_set_tx_power(int level);
void lora_set_spreading_factor(int sf);
void lora_set_signal_bandwidth(long sbw);
void lora_set_coding_rate4(int denominator);
void lora_receive(void);

/**
 * @brief Start a packet transmission.
 * @param header_length not used explicitly in this driver variant but kept for
 * compatibility
 */
int lora_begin_packet(int implicitHeader);

/**
 * @brief End a packet transmission (blocking).
 */
int lora_end_packet(void);

/**
 * @brief Write data to the FIFO.
 */
size_t lora_write(const uint8_t *buffer, size_t size);

/**
 * @brief Check if packet received, returns size of packet
 */
int lora_parse_packet(void);

/**
 * @brief Read bytes from received packet
 */
size_t lora_read(uint8_t *buffer, size_t size);

/**
 * @brief Put to sleep
 */
void lora_sleep(void);

/**
 * @brief Put to stand-by
 */
void lora_idle(void);

#endif
