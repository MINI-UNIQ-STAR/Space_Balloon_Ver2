#include "pps_capture.h"
#include "main.h"
#include "bsp.h"
#include <stdbool.h>

// 1PPS Synchronization State
static volatile uint32_t last_pps_tick = 0;
static volatile uint8_t current_slot = 0;
static volatile bool pps_synced = false;

// 50Hz telemetry = 20ms period
// We divide the 1-second period into 50 slots of 20ms each
#define SLOT_PERIOD_MS  20
#define SLOTS_PER_SECOND 50

void PPS_Init(void) {
    // EXTI4 is already configured in CubeMX for XA1110_PPS_Pin (PB4)
    // Just initialize state variables
    last_pps_tick = 0;
    current_slot = 0;
    pps_synced = false;
}

uint8_t PPS_GetSlot(void) {
    if (!pps_synced) {
        return 0xFF;  // Not synced
    }

    // Calculate current slot based on time since last PPS
    uint32_t elapsed_ms = BSP_GetTick() - last_pps_tick;

    // If we're past 1 second, we lost sync
    if (elapsed_ms >= 1000) {
        pps_synced = false;
        return 0xFF;
    }

    // Calculate slot number (0-49)
    current_slot = (uint8_t)(elapsed_ms / SLOT_PERIOD_MS);

    // Safety clamp
    if (current_slot >= SLOTS_PER_SECOND) {
        current_slot = SLOTS_PER_SECOND - 1;
    }

    return current_slot;
}

bool PPS_IsSynced(void) {
    // Check if we received a pulse recently (within 1.1 seconds)
    uint32_t elapsed_ms = BSP_GetTick() - last_pps_tick;
    if (elapsed_ms > 1100) {
        pps_synced = false;
    }
    return pps_synced;
}

uint32_t PPS_GetTimeSinceLastPulse_us(void) {
    if (last_pps_tick == 0) {
        return 0xFFFFFFFF;  // No pulse received yet
    }

    uint32_t elapsed_ms = BSP_GetTick() - last_pps_tick;
    return elapsed_ms * 1000;  // Convert to microseconds
}

/**
 * @brief EXTI4 Interrupt Handler (GPS_PPS)
 * @note This is called from stm32g4xx_it.c
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == XA1110_PPS_Pin) {
        // Rising edge detected on 1PPS signal
        last_pps_tick = BSP_GetTick();
        current_slot = 0;  // Reset to slot 0 at PPS edge
        pps_synced = true;
    }
}
