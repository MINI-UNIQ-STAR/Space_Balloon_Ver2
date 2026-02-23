/**
 * @file pps_capture.c
 * @brief GPS 1PPS 신호 캡처 및 동기화
 * @details GPS 1PPS (Pulse Per Second) 신호를 사용한 시간 동기화
 *          - 입력: XA1110_PPS 핀 (PB4, EXTI4)
 *          - 주기: 1초 (1Hz)
 *          - 슬롯: 50개 슬롯 (20ms 간격, 50Hz 텔레메트리 동기화)
 *          - 정확도: ±100ns (GPS 1PPS 정밀도)
 *          - 타임아웃: 1.1초 (동기화 손실 판정)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "pps_capture.h"
#include "main.h"
#include "bsp.h"
#include <stdbool.h>

/** @brief 마지막 PPS 펄스 수신 시각 (시스템 틱) */
static volatile uint32_t last_pps_tick = 0;

/** @brief 현재 슬롯 번호 (0-49) */
static volatile uint8_t current_slot = 0;

/** @brief PPS 동기화 상태 플래그 */
static volatile bool pps_synced = false;

/** @brief 슬롯 주기 (20ms) */
#define SLOT_PERIOD_MS  20

/** @brief 초당 슬롯 개수 (50Hz) */
#define SLOTS_PER_SECOND 50

/**
 * @brief PPS 캡처 초기화
 * @details EXTI4 인터럽트는 CubeMX에서 설정됨
 *          상태 변수만 초기화
 */
void PPS_Init(void) {
    // EXTI4 is already configured in CubeMX for XA1110_PPS_Pin (PB4)
    // Just initialize state variables
    last_pps_tick = 0;
    current_slot = 0;
    pps_synced = false;
}

/**
 * @brief 현재 슬롯 번호 조회
 * @return uint8_t 슬롯 번호 (0-49), 동기화 실패 시 0xFF
 * @details 마지막 PPS 펄스 이후 경과 시간을 기반으로 슬롯 계산
 *          슬롯 번호 = (경과 시간 ms) / 20ms
 */
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

/**
 * @brief PPS 동기화 상태 확인
 * @return bool true = 동기화됨, false = 동기화 손실
 * @details 마지막 펄스로부터 1.1초 이내면 동기화 상태로 간주
 */
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
