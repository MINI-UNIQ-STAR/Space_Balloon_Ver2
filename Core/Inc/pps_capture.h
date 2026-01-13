/**
 * @file pps_capture.h
 * @brief GPS 1PPS 동기화 인터페이스 - 시간 정렬 및 슬롯 분할
 * @details XA1110 GPS 1PPS 신호 (1초당 1펄스) 캡처 및 50Hz 슬롯 분할
 *          하드웨어: PB4 (EXTI4, Rising Edge), GPS_PPS 신호
 *          목적: 텔레메트리 전송 타이밍 정렬, 시간 동기화
 *          슬롯 분할: 1초를 50개 슬롯으로 분할 (20ms 간격, 0-49 슬롯)
 *          정밀도: 마이크로초 단위 (타이머 기반 측정)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef PPS_CAPTURE_H
#define PPS_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup PPS_FUNCTIONS GPS 1PPS 동기화 함수
 * @{
 */

/**
 * @brief 1PPS 캡처 시스템 초기화
 * @details 초기화 순서:
 *          1. EXTI4 인터럽트 활성화 (PB4, Rising Edge)
 *          2. 타이머 초기화 (마이크로초 측정용, TIM6 또는 TIM7)
 *          3. 동기화 상태 초기화 (Unsync)
 *          하드웨어 설정:
 *          - GPIO: PB4, Input Mode, Pull-Down
 *          - EXTI Line: 4, Rising Edge Trigger
 *          - 인터럽트 우선순위: PreemptPriority=2, SubPriority=0
 * @note App_Init()에서 1회 호출
 *       GPS Fix 획득 후 1PPS 신호 활성화 (Cold Start 최대 30초)
 */
void PPS_Init(void);

/**
 * @brief 현재 1PPS 슬롯 번호 조회
 * @return uint8_t 슬롯 번호 (0-49), 동기화 실패 시 0xFF
 * @details 슬롯 계산:
 *          - 1초 = 50 슬롯 (50Hz 메인 루프)
 *          - 각 슬롯 = 20ms 간격
 *          - 슬롯 번호 = (마지막 PPS 이후 경과 시간 / 20ms) % 50
 *          사용처: App_Loop()에서 텔레메트리 전송 타이밍 결정
 *          예: 슬롯 0에서 센서 데이터 전송 시작
 * @note 동기화 타임아웃: 1.5초 (마지막 PPS 펄스 이후)
 *       타임아웃 발생 시 0xFF 반환, PPS_IsSynced() = false
 */
uint8_t PPS_GetSlot(void);

/**
 * @brief 1PPS 동기화 상태 확인
 * @return bool true=동기화됨, false=동기화 실패/타임아웃
 * @details 동기화 조건:
 *          - 마지막 PPS 펄스 수신 후 1.5초 이내
 *          - EXTI4 인터럽트 최소 1회 발생
 *          동기화 해제 조건:
 *          - GPS Fix 상실 (GPS_fix = 0)
 *          - 1.5초 이상 PPS 펄스 미수신 (타임아웃)
 * @note App_Loop()에서 주기적으로 확인
 *       동기화 실패 시 내부 시스템 틱(HAL_GetTick)으로 폴백
 */
bool PPS_IsSynced(void);

/**
 * @brief 마지막 PPS 펄스 이후 경과 시간 조회
 * @return uint32_t 경과 시간 (µs), PPS 펄스 미수신 시 0xFFFFFFFF
 * @details 측정 메커니즘:
 *          - EXTI4 인터럽트에서 타이머 카운터 리셋
 *          - 타이머 주파수: 1MHz (1µs 분해능)
 *          - 측정 범위: 0 ~ 4,294,967,295 µs (~71.6분)
 *          사용처: 슬롯 번호 계산 (PPS_GetSlot 내부 호출)
 * @note 타이머 오버플로우 방지: 1초마다 PPS 펄스로 자동 리셋
 *       정밀도: ±1µs (타이머 클럭 안정성에 의존)
 */
uint32_t PPS_GetTimeSinceLastPulse_us(void);

/** @} */ // end of PPS_FUNCTIONS

#endif /* PPS_CAPTURE_H */
