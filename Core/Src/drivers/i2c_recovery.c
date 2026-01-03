#include "drivers/i2c_recovery.h"

#include "drivers/dwt_delay.h"

static void bus_clear(GPIO_TypeDef *scl_port,
                      uint16_t scl_pin,
                      GPIO_TypeDef *sda_port,
                      uint16_t sda_pin)
{
	GPIO_InitTypeDef init = {0};

	// Configure as open-drain outputs so we can clock out any stuck slave.
	init.Mode = GPIO_MODE_OUTPUT_OD;
	init.Pull = GPIO_PULLUP;
	init.Speed = GPIO_SPEED_FREQ_LOW;

	init.Pin = scl_pin;
	HAL_GPIO_Init(scl_port, &init);
	init.Pin = sda_pin;
	HAL_GPIO_Init(sda_port, &init);

	// Release both lines high.
	HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_SET);
	dwt_delay_us(5);

	// If SDA is stuck low, toggle SCL 9 times.
	for (uint8_t i = 0; i < 9u; i++) {
		HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
		dwt_delay_us(5);
		HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_RESET);
		dwt_delay_us(5);
	}

	// Generate a STOP: SCL high, then SDA high.
	HAL_GPIO_WritePin(scl_port, scl_pin, GPIO_PIN_SET);
	dwt_delay_us(5);
	HAL_GPIO_WritePin(sda_port, sda_pin, GPIO_PIN_SET);
	dwt_delay_us(5);
}

bool i2c_recover_bus(I2C_HandleTypeDef *hi2c,
                     GPIO_TypeDef *scl_port,
                     uint16_t scl_pin,
                     GPIO_TypeDef *sda_port,
                     uint16_t sda_pin)
{
	if ((hi2c == NULL) || (scl_port == NULL) || (sda_port == NULL)) {
		return false;
	}

	(void)dwt_delay_init();

	(void)HAL_I2C_DeInit(hi2c);
	bus_clear(scl_port, scl_pin, sda_port, sda_pin);

	// Re-init restores AF pin config via MSP.
	if (HAL_I2C_Init(hi2c) != HAL_OK) {
		return false;
	}

	(void)HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE);
	(void)HAL_I2CEx_ConfigDigitalFilter(hi2c, 0);
	return true;
}
