#include "main.h"
#include "sensors.h"
#include "unity.h"
#include <stdio.h>

// --- Mock HAL 상태 (시간) ---
/** @brief 시뮬레이션된 현재 시간 (ms) */
static uint32_t mock_tick_ms = 0;

/** @brief 시간 설정 */
void MockHAL_SetTick(uint32_t tick) { mock_tick_ms = tick; }

/** @brief 시간 전진 */
void MockHAL_AdvanceTick(uint32_t delta) { mock_tick_ms += delta; }

/** @brief HAL_GetTick Mock 구현 */
uint32_t HAL_GetTick(void) { return mock_tick_ms; }

// --- Mock GPIO 상태 ---
/** @brief PMS SET 핀의 마지막 상태 저장 변수 */
static int pms_set_state = -1;

/** @brief HAL_GPIO_WritePin Mock 구현 - PMS 핀 상태 캡처 */
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                       GPIO_PinState PinState) {
  // PMS SET 핀 제어 여부 확인
  // GPIOB는 실제 STM32에서 0x48000400, mock에서 0xB
  // 두 주소 모두 PMS_SET 핀으로 간주
  uintptr_t port_addr = (uintptr_t)GPIOx;
  bool is_gpio_b = (port_addr == 0xB || port_addr == 0x48000400);

  if (is_gpio_b && GPIO_Pin == GPIO_PIN_10) {
    pms_set_state = PinState;
  }
}

/** @brief PMS SET 핀 상태 조회 Helper */
int MockGPIO_GetPMSSetState(void) { return pms_set_state; }

// --- Mock Sensors 상태 ---
static int sensors_reset_call_count = 0;  /**< 리셋 호출 횟수 */
static SensorID_t last_reset_sensor = -1; /**< 마지막 리셋 센서 ID */

#ifndef INTEGRATION_TEST
/**
 * @brief 센서 리셋 (Mock)
 * @details 실제 리셋 대신 호출 횟수와 ID를 기록하여 테스트 검증에 사용
 */
void Sensors_Reset(SensorID_t id) {
  sensors_reset_call_count++;
  last_reset_sensor = id;
}
#endif

/** @brief 모든 Mock 상태 및 통계 초기화 */
void MockSensors_ClearStats(void) {
  sensors_reset_call_count = 0;
  last_reset_sensor = -1;
  pms_set_state = -1;
  mock_tick_ms = 0;
}

/** @brief 리셋 호출 횟수 조회 Helper */
int MockSensors_GetResetCount(void) { return sensors_reset_call_count; }

/** @brief 마지막 리셋 센서 ID 조회 Helper */
int MockSensors_GetLastResetSensor(void) { return (int)last_reset_sensor; }

/** @brief 센서 리셋 처리 (Mock) */
void Sensors_ProcessReset(void) {
  // Non-blocking reset state machine - no-op for unit tests
}
