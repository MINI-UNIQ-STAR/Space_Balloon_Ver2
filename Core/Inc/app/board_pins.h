#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stm32g4xx_hal.h"

/*
 * Board-specific GPIO mappings for sensor reset / power control lines.
 *
 * Define the macros below based on your CubeMX pin assignments.
 *
 * Example:
 *   #define RESET_LSM_RST_GPIO_PORT GPIOB
 *   #define RESET_LSM_RST_GPIO_PIN  GPIO_PIN_10
 */

// From schematic page 1 (WeAct module pin labels):
//   PMS_SET -> B10 (PB10)
//   LSM_RST -> B11 (PB11)
//   CO2_RST -> B0  (PB0)
//   MS_RST  -> A5  (PA5)
//   MCP_RST -> A4  (PA4)
// SEN_RST is mapped to PB1 in the schematic and is configured as GPIO_Output in .ioc.

#define RESET_LSM_RST_GPIO_PORT GPIOB
#define RESET_LSM_RST_GPIO_PIN  GPIO_PIN_11

// #define RESET_MLX_RST_GPIO_PORT GPIOB
// #define RESET_MLX_RST_GPIO_PIN  GPIO_PIN_11

// CO2 reset line
#define RESET_CO2_RST_GPIO_PORT GPIOB
#define RESET_CO2_RST_GPIO_PIN  GPIO_PIN_0

// GDK101 power control (P-MOS). HIGH = OFF
#define RESET_GDK101_PWR_GPIO_PORT GPIOB
#define RESET_GDK101_PWR_GPIO_PIN  GPIO_PIN_2

// Sensor reset/power control line
#define RESET_SEN_RST_GPIO_PORT GPIOB
#define RESET_SEN_RST_GPIO_PIN  GPIO_PIN_1

// MS5611 reset line
#define RESET_MS_RST_GPIO_PORT GPIOA
#define RESET_MS_RST_GPIO_PIN  GPIO_PIN_5

// MCP9600 reset line
#define RESET_MCP_RST_GPIO_PORT GPIOA
#define RESET_MCP_RST_GPIO_PIN  GPIO_PIN_4

// SHT31 reset line (using PB13 - previously unused GPIO output)
#define RESET_SHT_RST_GPIO_PORT GPIOB
#define RESET_SHT_RST_GPIO_PIN  GPIO_PIN_13

// PMS3003 SET pin
#define RESET_PMS_SET_GPIO_PORT GPIOB
#define RESET_PMS_SET_GPIO_PIN  GPIO_PIN_10

// MLX90393 reset (active-low assumed). Mapped onto an otherwise-unused GPIO output.
#define RESET_MLX_RST_GPIO_PORT GPIOB
#define RESET_MLX_RST_GPIO_PIN  GPIO_PIN_14

// GPS 1PPS input (from schematic net GPS_PPS / XA1110_1PPS)
#define GPS_PPS_GPIO_PORT GPIOB
#define GPS_PPS_GPIO_PIN  GPIO_PIN_4

// GPS interrupt input (exact semantics TBD; used for feature/status interrupts)
#define GPS_INT_GPIO_PORT GPIOB
#define GPS_INT_GPIO_PIN  GPIO_PIN_12

// LSM6DSV16x interrupt input (PB6)
#define LSM_INT_GPIO_PORT GPIOB
#define LSM_INT_GPIO_PIN  GPIO_PIN_6

// MLX90393 interrupt input (PB7)
#define MLX_INT_GPIO_PORT GPIOB
#define MLX_INT_GPIO_PIN  GPIO_PIN_7

// GPS wake control (active level TBD; treated as digital output)
#define GPS_WAKE_GPIO_PORT GPIOA
#define GPS_WAKE_GPIO_PIN  GPIO_PIN_7

// GPS reset control (active-low; deasserted high)
#define GPS_nRST_GPIO_PORT GPIOA
#define GPS_nRST_GPIO_PIN  GPIO_PIN_9

#endif /* BOARD_PINS_H */
