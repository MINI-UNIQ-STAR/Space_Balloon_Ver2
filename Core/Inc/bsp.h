/*
 * bsp.h
 *
 *  Created on: Jan 8, 2026
 *      Author: SpaceBalloon Team
 */

#ifndef BSP_H_
#define BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* --- Hardware Definitions --- */

// Sensor Addresses (I2C1 - Downside)
#define BSP_LSM6DSV16X_ADDR     0x6B
#define BSP_MLX90393_ADDR       0x0C 
#define BSP_GDK101_ADDR         0x18 

// Sensor Addresses (I2C3 - Upside)
#define BSP_SHT31_ADDR          0x44 
#define BSP_MS5611_ADDR         0x77 
#define BSP_CM1107N_ADDR        0x31 
#define BSP_MCP9600_ADDR        0x60 // Default 0x60 or 0x67 check config
#define BSP_SEN0321_ADDR        0x70 // Check specific address

/* --- Public Functions --- */

/**
 * @brief Initialize Board Support Package (GPIOs, Peripherals are assumed init by HAL/Main)
 */
void BSP_Init(void);

/**
 * @brief Enable Power/Signals to Sensors (GPIO sequencing)
 */
void BSP_Sensor_PowerOn(void);

/**
 * @brief I2C1 (Downside) Register Write
 */
int32_t BSP_I2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C1 (Downside) Register Read
 */
int32_t BSP_I2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C1 (Downside) Raw Write
 */
int32_t BSP_I2C1_Write(uint16_t DevAddr, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C1 (Downside) Raw Read
 */
int32_t BSP_I2C1_Read(uint16_t DevAddr, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C3 (Upside) Register Write
 */
int32_t BSP_I2C3_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief I2C3 (Upside) Register Read
 */
int32_t BSP_I2C3_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len);

/**
 * @brief UART Write (Abstracted)
 */
int32_t BSP_UART_Write(uint8_t *pData, uint16_t Len);

/**
 * @brief UART Read (Abstracted)
 */
int32_t BSP_UART_Read(uint8_t *pData, uint16_t Len);

/**
 * @brief Get System Tick (ms)
 */
uint32_t BSP_GetTick(void);

/**
 * @brief Delay (ms)
 */
void BSP_Delay(uint32_t Delay);

/**
 * @brief ADC Read Battery Voltage (mV)
 */
uint16_t BSP_ADC_Read_Battery_mV(void);

/**
 * @brief I2C1 Bus Recovery (9-Clock Pulse)
 * @note Implements clock stretching recovery by generating 9 clock pulses on SCL line
 */
void BSP_I2C1_Recovery(void);

/**
 * @brief I2C3 Bus Recovery (9-Clock Pulse)
 * @note Implements clock stretching recovery by generating 9 clock pulses on SCL line
 */
void BSP_I2C3_Recovery(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_H_ */
