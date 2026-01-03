#ifndef RESET_LINES_H
#define RESET_LINES_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32g4xx_hal.h"
#include "app/board_pins.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	RESET_LINE_LSM_RST = 0,
	RESET_LINE_MLX_RST,
	RESET_LINE_CO2_RST,
	RESET_LINE_SEN_RST,
	RESET_LINE_MS_RST,
	RESET_LINE_MCP_RST,
	RESET_LINE_SHT_RST,
	RESET_LINE_PMS_SET,
	RESET_LINE_COUNT
} reset_line_t;

// Set a configured line to a steady level (e.g. PMS_SET LOW = off).
// Returns false if the line is not mapped.
bool reset_line_set(reset_line_t line, bool level_high);

/**
 * @brief Drive a LOW->HIGH->LOW pulse on a configured reset line.
 *
 * This function is a no-op (returns false) unless the corresponding
 * RESET_*_GPIO_PORT and RESET_*_GPIO_PIN macros are defined.
 */
bool reset_line_pulse(reset_line_t line, uint32_t low_ms, uint32_t high_ms, uint32_t low2_ms);

// Drive a HIGH->LOW->HIGH pulse (used by some control lines like PMS_SET).
// Returns false if the line is not mapped.
bool reset_line_pulse_high_low_high(reset_line_t line, uint32_t high_ms, uint32_t low_ms, uint32_t high2_ms);

#ifdef __cplusplus
}
#endif

#endif /* RESET_LINES_H */
