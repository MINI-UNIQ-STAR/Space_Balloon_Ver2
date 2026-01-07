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
    // return HAL_UART_Transmit(&huart3, pData, Len, 100);
    return 0;
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
