/**
 * @file kalman.h
 * @brief 칼만 필터 인터페이스 - 고도 추정용
 * @details 1차원 칼만 필터 (고도, 수직 속도)
 *          입력: MS5611 기압 고도
 *          출력: 필터링된 고도 및 수직 속도 추정
 *          발산 보호: 공분산 리셋 (P > 10000 또는 NaN)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#ifndef __KALMAN_H
#define __KALMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "bsp.h" /* For float32_t definition */

/**
 * @brief 칼만 필터 핸들 구조체
 * @details 2×2 상태 벡터 [고도, 수직 속도]
 *          상수 속도 모델 (Constant Velocity Model)
 *          측정 모델: H = [1 0] (기압계는 고도만 측정)
 */
typedef struct {
    float32_t x[2];          /**< 상태 벡터 [0]=고도(m), [1]=수직 속도(m/s) */
    float32_t P[2][2];       /**< 공분산 행렬 (State Covariance) */
    float32_t Q[2][2];       /**< 프로세스 노이즈 공분산 (Process Noise Covariance) */
    float32_t R;             /**< 측정 노이즈 공분산 (Measurement Noise Covariance, 스칼라) */
    float32_t dt;            /**< 샘플링 주기 (초, 예: 0.02s = 50Hz) */
} KF_Handle_t;

/**
 * @defgroup KF_FUNCTIONS 칼만 필터 함수
 * @{
 */

/**
 * @brief 칼만 필터 초기화
 * @param[in,out] hkf 칼만 필터 핸들 포인터
 * @param[in] dt 샘플링 주기 (초, 예: 0.02s = 50Hz)
 * @param[in] process_noise 프로세스 노이즈 (예: 0.5, 풍선 동역학 기반)
 * @param[in] meas_noise 측정 노이즈 (예: 0.3, MS5611 정밀도 ~0.5m 기반)
 * @details 초기 상태:
 *          - x[0] = 0.0m (고도)
 *          - x[1] = 0.0m/s (수직 속도)
 *          - P = 단위 행렬 (초기 공분산 불확실성 낮음)
 *          - Q = process_noise × [dt^2/2, dt; dt, 1] (상수 속도 모델)
 *          - R = meas_noise (기압계 측정 노이즈)
 * @note App_Init()에서 1회 호출
 *       권장 설정: KF_Init(&hkf, 0.02f, 0.5f, 0.3f)
 */
void KF_Init(KF_Handle_t *hkf, float32_t dt, float32_t process_noise, float32_t meas_noise);

/**
 * @brief 칼만 필터 예측 단계 (시간 업데이트)
 * @param[in,out] hkf 칼만 필터 핸들 포인터
 * @details 상태 방정식:
 *          - x_pred = F × x
 *            F = [1 dt]  (상수 속도 모델)
 *                [0  1]
 *          - P_pred = F × P × F^T + Q
 * @note App_Loop()에서 KF_Update_Altitude() 전에 호출
 */
void KF_Predict(KF_Handle_t *hkf);

/**
 * @brief 칼만 필터 업데이트 단계 (측정 업데이트)
 * @param[in,out] hkf 칼만 필터 핸들 포인터
 * @param[in] measurement 기압 고도 측정값 (m, MS5611)
 * @details 측정 방정식:
 *          - y = z - H × x_pred (잔차, z=measurement, H=[1 0])
 *          - S = H × P_pred × H^T + R (잔차 공분산)
 *          - K = P_pred × H^T × S^-1 (칼만 이득)
 *          - x = x_pred + K × y (상태 업데이트)
 *          - P = (I - K × H) × P_pred (공분산 업데이트)
 * @note App_Loop()에서 KF_Predict() 후 호출
 *       업데이트 후 발산 체크 (KF_CheckDivergence) 권장
 */
void KF_Update_Altitude(KF_Handle_t *hkf, float32_t measurement);

/**
 * @brief 칼만 필터 발산 체크 및 복구
 * @param[in,out] hkf 칼만 필터 핸들 포인터
 * @details 발산 조건:
 *          - 공분산 P의 대각 요소 > 10000 (과도한 불확실성)
 *          - NaN 발생 (수치 불안정성)
 *          발산 감지 시 동작:
 *          - P 행렬을 단위 행렬로 리셋 (공분산 초기화)
 *          - 상태 벡터 x는 유지 (현재 추정값 보존)
 * @note KF_Update_Altitude() 후 호출 권장
 *       MISRA C 준수 (NaN 체크, 분리된 할당)
 */
void KF_CheckDivergence(KF_Handle_t *hkf);

/** @} */ // end of KF_FUNCTIONS

#ifdef __cplusplus
}
#endif

#endif /* __KALMAN_H */
