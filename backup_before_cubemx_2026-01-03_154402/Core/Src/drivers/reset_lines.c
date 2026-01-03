#include "drivers/reset_lines.h"

static bool reset_line_get_gpio(reset_line_t line, GPIO_TypeDef **port, uint16_t *pin)
{
	switch (line)
	{
		case RESET_LINE_LSM_RST:
			#ifdef RESET_LSM_RST_GPIO_PORT
			*port = RESET_LSM_RST_GPIO_PORT;
			*pin = RESET_LSM_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_MLX_RST:
			#ifdef RESET_MLX_RST_GPIO_PORT
			*port = RESET_MLX_RST_GPIO_PORT;
			*pin = RESET_MLX_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_CO2_RST:
			#ifdef RESET_CO2_RST_GPIO_PORT
			*port = RESET_CO2_RST_GPIO_PORT;
			*pin = RESET_CO2_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_SEN_RST:
			#ifdef RESET_SEN_RST_GPIO_PORT
			*port = RESET_SEN_RST_GPIO_PORT;
			*pin = RESET_SEN_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_MS_RST:
			#ifdef RESET_MS_RST_GPIO_PORT
			*port = RESET_MS_RST_GPIO_PORT;
			*pin = RESET_MS_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_MCP_RST:
			#ifdef RESET_MCP_RST_GPIO_PORT
			*port = RESET_MCP_RST_GPIO_PORT;
			*pin = RESET_MCP_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_SHT_RST:
			#ifdef RESET_SHT_RST_GPIO_PORT
			*port = RESET_SHT_RST_GPIO_PORT;
			*pin = RESET_SHT_RST_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		case RESET_LINE_PMS_SET:
			#ifdef RESET_PMS_SET_GPIO_PORT
			*port = RESET_PMS_SET_GPIO_PORT;
			*pin = RESET_PMS_SET_GPIO_PIN;
			return true;
			#else
			(void)port;
			(void)pin;
			return false;
			#endif

		default:
			return false;
	}
}

bool reset_line_pulse(reset_line_t line, uint32_t low_ms, uint32_t high_ms, uint32_t low2_ms)
{
	GPIO_TypeDef *port = NULL;
	uint16_t pin = 0;
	if (!reset_line_get_gpio(line, &port, &pin))
	{
		return false;
	}

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
	if (low_ms)
	{
		HAL_Delay(low_ms);
	}

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
	if (high_ms)
	{
		HAL_Delay(high_ms);
	}

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
	if (low2_ms)
	{
		HAL_Delay(low2_ms);
	}

	return true;
}

bool reset_line_set(reset_line_t line, bool level_high)
{
	GPIO_TypeDef *port = NULL;
	uint16_t pin = 0;
	if (!reset_line_get_gpio(line, &port, &pin)) {
		return false;
	}
	HAL_GPIO_WritePin(port, pin, level_high ? GPIO_PIN_SET : GPIO_PIN_RESET);
	return true;
}

bool reset_line_pulse_high_low_high(reset_line_t line, uint32_t high_ms, uint32_t low_ms, uint32_t high2_ms)
{
	GPIO_TypeDef *port = NULL;
	uint16_t pin = 0;
	if (!reset_line_get_gpio(line, &port, &pin))
	{
		return false;
	}

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
	if (high_ms) {
		HAL_Delay(high_ms);
	}

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
	if (low_ms) {
		HAL_Delay(low_ms);
	}

	HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
	if (high2_ms) {
		HAL_Delay(high2_ms);
	}

	return true;
}
