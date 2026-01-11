/*
 * bsp.c
 *
 *  Created on: Jan 8, 2026
 *      Author: SpaceBalloon Team
 */

#include "bsp.h"
#include "main.h" // HAL Includes
#include <string.h> // For memset, memcpy
#include <stdio.h>  // For printf (mock)

// Mock State for Simulation
// static float mock_altitude_bsp = 100.0f; // Unused warning

// Forward declaration of private helper functions
static void BSP_Delay_us(uint32_t us);

void BSP_Init(void) {
    // Hardware Init is mostly done in main.c (HAL_Init, SystemClock, MX_...)
    // Here we can do specific board level setup if needed
}

void BSP_Sensor_PowerOn(void) {
#ifndef UNIT_TEST
    // Release Resets
    // Using names from main.h
    HAL_GPIO_WritePin(XA1110_RST_GPIO_Port, XA1110_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(XA1110_Wake_GPIO_Port, XA1110_Wake_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PMS_SET_GPIO_Port, PMS_SET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MCP_RST_GPIO_Port, MCP_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MS_RST_GPIO_Port, MS_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(CM1107N_RST_GPIO_Port, CM1107N_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SEN_RST_GPIO_Port, SEN_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MLX_RST_GPIO_Port, MLX_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(100); 
#endif
}

// --- I2C1 (Downside) Implementation ---
extern I2C_HandleTypeDef hi2c1;

int32_t BSP_I2C1_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    return HAL_I2C_Mem_Write(&hi2c1, DevAddr, Reg, I2C_MEMADD_SIZE_8BIT, pData, Len, 1000);
#else
    return 0; // Mock Success
#endif
}

int32_t BSP_I2C1_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    return HAL_I2C_Mem_Read(&hi2c1, DevAddr, Reg, I2C_MEMADD_SIZE_8BIT, pData, Len, 1000);
#else
    // Mock Logic moved from sensors.c
    memset(pData, 0, Len);
    
    // LSM6DSV16X Mocking
    if (DevAddr == BSP_LSM6DSV16X_ADDR) { 
         if (Reg == 0x0F) { // WHO_AM_I
             pData[0] = 0x70; // LSM6DSV16X_ID
         }
         else if (Reg == 0x20) { // OUTZ_L_A like logic
             int16_t val = 16384; // 1G
             pData[0] = (uint8_t)(val & 0xFF);
             if (Len > 1) pData[1] = (uint8_t)((val >> 8) & 0xFF);
         }
    }
    return 0;
#endif
}

int32_t BSP_I2C1_Write(uint16_t DevAddr, uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    return HAL_I2C_Master_Transmit(&hi2c1, DevAddr, pData, Len, 1000);
#else
    return 0;
#endif
}

int32_t BSP_I2C1_Read(uint16_t DevAddr, uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    return HAL_I2C_Master_Receive(&hi2c1, DevAddr, pData, Len, 1000);
#else
    memset(pData, 0, Len);
    return 0;
#endif
}


// --- I2C3 (Upside) Implementation ---
extern I2C_HandleTypeDef hi2c3;

int32_t BSP_I2C3_WriteReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    return HAL_I2C_Mem_Write(&hi2c3, DevAddr, Reg, I2C_MEMADD_SIZE_8BIT, pData, Len, 1000);
#else
    return 0;
#endif
}

int32_t BSP_I2C3_ReadReg(uint16_t DevAddr, uint16_t Reg, uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    return HAL_I2C_Mem_Read(&hi2c3, DevAddr, Reg, I2C_MEMADD_SIZE_8BIT, pData, Len, 1000);
#else
    memset(pData, 0, Len);
    return 0; 
#endif
}


// --- UART Implementation ---
extern UART_HandleTypeDef huart3; // Use UART3 for Sensors/PMS?

int32_t BSP_UART_Write(uint8_t *pData, uint16_t Len) {
#ifndef UNIT_TEST
    // Assuming UART3 for general sensor bus or debug
    return HAL_UART_Transmit(&huart3, pData, Len, 100);
    // return 0;
#else
    // Mock Print
    char tmp[128];
    if (Len < 128) {
        memcpy(tmp, pData, Len);
        tmp[Len] = 0;
        if (tmp[0] == '$') printf("UART TX: %s", tmp);
        else printf("UART TX: [Binary %d bytes]\n", Len);
    }
    return 0;
#endif
}

int32_t BSP_UART_Read(uint8_t *pData, uint16_t Len) {
    // Mock Response Generator
    if (Len >= 8) {
        // e.g. CM1107N Response
        pData[0] = 0x16;
        pData[1] = 0x05;
        pData[2] = 0x01;
        pData[3] = 0x01; 
        pData[4] = 0xF4; 
        pData[5] = 0x00;
        pData[6] = 0x00;
        uint16_t sum = 0;
        for(int i=0; i<7; i++) sum += pData[i];
        pData[7] = (256 - (sum % 256)) % 256;
    }
    return 0;
}

// --- System ---
uint32_t BSP_GetTick(void) {
    return HAL_GetTick();
}

void BSP_Delay(uint32_t Delay) {
    HAL_Delay(Delay);
}


// --- ADC ---
extern ADC_HandleTypeDef hadc1;

uint16_t BSP_ADC_Read_Battery_mV(void) {
#ifndef UNIT_TEST
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        float voltage_mv = (raw * 3300.0f / 4096.0f) * 6.0f;
        HAL_ADC_Stop(&hadc1);
        return (uint16_t)voltage_mv;
    }
    HAL_ADC_Stop(&hadc1);
    return 0;
#else
    return 4200;
#endif
}

// --- I2C Bus Recovery Implementation ---

/**
 * @brief I2C1 Bus Recovery using 9-Clock Pulse method
 * @note Implements I2C bus recovery by:
 *       1. Disabling I2C peripheral
 *       2. Reconfiguring SCL as GPIO output
 *       3. Generating 9 clock pulses (100 kHz)
 *       4. Reconfiguring SCL back to I2C alternate function
 *       5. Re-enabling I2C peripheral
 */
void BSP_I2C1_Recovery(void) {
#ifndef UNIT_TEST
    // I2C1: PA15 (SCL), PB9 (SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Step 1: Disable I2C peripheral
    HAL_I2C_DeInit(&hi2c1);

    // Step 2: Configure SCL (PA15) as GPIO Output (Open-Drain)
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Step 3: Generate 9 clock pulses (100 kHz = 10us period, 5us high, 5us low)
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET); // SCL Low
        BSP_Delay_us(5);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);   // SCL High
        BSP_Delay_us(5);
    }

    // Step 4: Reconfigure SCL back to I2C alternate function
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1; // Check from main.h or ioc
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Step 5: Re-enable I2C peripheral
    HAL_I2C_Init(&hi2c1);

    // Small delay for stabilization
    HAL_Delay(10);
#endif
}

/**
 * @brief I2C3 Bus Recovery using 9-Clock Pulse method
 */
void BSP_I2C3_Recovery(void) {
#ifndef UNIT_TEST
    // I2C3: PA8 (SCL), PB5 (SDA)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Step 1: Disable I2C peripheral
    HAL_I2C_DeInit(&hi2c3);

    // Step 2: Configure SCL (PA8) as GPIO Output (Open-Drain)
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Step 3: Generate 9 clock pulses
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET); // SCL Low
        BSP_Delay_us(5);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);   // SCL High
        BSP_Delay_us(5);
    }

    // Step 4: Reconfigure SCL back to I2C alternate function
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_I2C3; // Check from main.h or ioc
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Step 5: Re-enable I2C peripheral
    HAL_I2C_Init(&hi2c3);

    // Small delay for stabilization
    HAL_Delay(10);
#endif
}

/**
 * @brief Microsecond delay helper
 * @param us Delay in microseconds
 * @note Uses DWT cycle counter for precise timing
 */
static void BSP_Delay_us(uint32_t us) {
#ifndef UNIT_TEST
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles);
#endif
}
