#ifndef __MOCK_DEPENDENCIES_H
#define __MOCK_DEPENDENCIES_H

#include <stdint.h>
#include "sensors.h"

// --- Mock 제어 및 상태 확인 함수 (Test Helper) ---

/** @brief Mock 시간(Tick) 강제 설정 */
void MockHAL_SetTick(uint32_t tick);

/** @brief Mock 시간(Tick) 전진 */
void MockHAL_AdvanceTick(uint32_t delta);

/** @brief Mock 센서/GPIO 관련 통계 및 상태 초기화 */
void MockSensors_ClearStats(void);

/** @brief 센서 리셋 함수 호출 횟수 반환 */
int MockSensors_GetResetCount(void);

/** @brief 마지막으로 리셋된 센서 ID 반환 */
int MockSensors_GetLastResetSensor(void);

/** @brief PMS(전원 관리) GPIO 설정 상태 반환 */
int MockGPIO_GetPMSSetState(void);


// --- 시스템/드라이버 Mock 구현 (FDIR 모듈이 호출하는 함수들) ---

/** @brief 현재 시스템 시간 반환 (Mock) */
uint32_t HAL_GetTick(void);

/** @brief 센서 리셋 수행 (Mock) */
void Sensors_Reset(SensorID_t id);

#endif
