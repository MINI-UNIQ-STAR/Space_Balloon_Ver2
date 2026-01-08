#ifndef PPS_CAPTURE_H
#define PPS_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize 1PPS capture system
 * @note Configures EXTI4 interrupt for GPS_PPS pin (PB4)
 */
void PPS_Init(void);

/**
 * @brief Get current 1PPS slot number (0-49 for 50Hz)
 * @return Slot number, or 0xFF if no sync
 */
uint8_t PPS_GetSlot(void);

/**
 * @brief Check if system is synchronized to 1PPS
 * @return true if synced, false otherwise
 */
bool PPS_IsSynced(void);

/**
 * @brief Get time since last PPS pulse (microseconds)
 * @return Time in microseconds, or 0xFFFFFFFF if no pulse received
 */
uint32_t PPS_GetTimeSinceLastPulse_us(void);

#endif /* PPS_CAPTURE_H */
