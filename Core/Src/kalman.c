/**
 * @file kalman.c
 * @brief 칼만 필터 구현 - 고도 추정 및 수직 속도 계산
 * @details 1차원 칼만 필터 (고도 및 수직 속도 추정)
 *          - 상태 변수: [고도, 수직속도]
 *          - 측정값: 기압 고도 (MS5611)
 *          - 예측 모델: 등속도 모델 (Constant Velocity Model)
 *          - 프로세스 노이즈: 0.5 (풍선 역학)
 *          - 측정 노이즈: 0.3 (기압계 정밀도 ~0.5m)
 *          - 발산 방지: 공분산 상한 (10000.0) 및 NaN 검출
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "kalman.h"

/** @brief MISRA C 호환 NaN 검사 매크로 (math.h 의존성 제거) */
#define KF_ISNAN(x) ((x) != (x))

#ifdef HOST_TEST_MODE
#include <stdio.h>
#endif

/**
 * @brief 칼만 필터 초기화
 * @param hkf 칼만 필터 핸들
 * @param dt 샘플 주기 (초, 50Hz = 0.02s)
 * @param process_noise 프로세스 노이즈 (Q)
 * @param meas_noise 측정 노이즈 (R)
 * @details 초기 상태:
 *          - 고도 = 0m, 속도 = 0m/s
 *          - 공분산 = 단위 행렬
 */
void KF_Init(KF_Handle_t *hkf, float dt, float process_noise, float meas_noise) {
    hkf->dt = dt;
    
    /* Initial State */
    hkf->x[0] = 0.0f;  /* Altitude */
    hkf->x[1] = 0.0f;  /* Velocity */
    
    /* Initial Covariance - MISRA: separate statements per line */
    hkf->P[0][0] = 1.0f;
    hkf->P[0][1] = 0.0f;
    hkf->P[1][0] = 0.0f;
    hkf->P[1][1] = 1.0f;
    
    /* Process Noise Q (Constant Velocity Model assumption)
     * Q = q * [dt^3/3  dt^2/2; dt^2/2  dt]
     * Simplified for now */
    hkf->Q[0][0] = process_noise;
    hkf->Q[0][1] = 0.0f;
    hkf->Q[1][0] = 0.0f;
    hkf->Q[1][1] = process_noise;
    
    hkf->R = meas_noise;
}

/**
 * @brief 칼만 필터 예측 단계
 * @param hkf 칼만 필터 핸들
 * @details 상태 전이 행렬 F = [1 dt; 0 1]
 *          - 상태 예측: x = F * x
 *          - 공분산 예측: P = F * P * F^T + Q
 */
void KF_Predict(KF_Handle_t *hkf) {
    /* F = [1 dt; 0 1] */
    float old_alt = hkf->x[0];
    float old_vel = hkf->x[1];
    
    /* State Prediction: x = F * x */
    hkf->x[0] = old_alt + (old_vel * hkf->dt);
    hkf->x[1] = old_vel;
    
    /* Covariance Prediction: P = F * P * F^T + Q */
    float p00 = hkf->P[0][0];
    float p01 = hkf->P[0][1];
    float p10 = hkf->P[1][0];
    float p11 = hkf->P[1][1];
    float dt = hkf->dt;
    
    hkf->P[0][0] = p00 + dt * (p10 + p01) + dt * dt * p11 + hkf->Q[0][0];
    hkf->P[0][1] = p01 + dt * p11 + hkf->Q[0][1];
    hkf->P[1][0] = p10 + dt * p11 + hkf->Q[1][0];
    hkf->P[1][1] = p11 + hkf->Q[1][1];
}

/**
 * @brief 칼만 필터 업데이트 단계 (고도 측정값)
 * @param hkf 칼만 필터 핸들
 * @param measurement 기압 고도 측정값 (m)
 * @details 측정 행렬 H = [1 0]
 *          - 칼만 이득: K = P * H^T * (H * P * H^T + R)^-1
 *          - 상태 업데이트: x = x + K * (z - H * x)
 *          - 공분산 업데이트: P = (I - K * H) * P
 * @note KF_Predict()를 먼저 호출해야 함
 */
void KF_Update_Altitude(KF_Handle_t *hkf, float measurement) {
    /* NOTE: KF_Predict() must be called manually by user before this function 
     * to avoid implicit recursion and strictly separate phases */
    
    /* H = [1 0], y = z - H * x */
    float y = measurement - hkf->x[0];
    
    /* S = H * P * H^T + R */
    float S = hkf->P[0][0] + hkf->R;
    
    /* K = P * H^T * S^-1 */
    float K0 = hkf->P[0][0] / S;
    float K1 = hkf->P[1][0] / S;
    
    /* Update State: x = x + K * y */
    hkf->x[0] = hkf->x[0] + (K0 * y);
    hkf->x[1] = hkf->x[1] + (K1 * y);
    
    /* Update Covariance: P = (I - K * H) * P */
    float p00 = hkf->P[0][0];
    float p01 = hkf->P[0][1];
    
    hkf->P[0][0] = hkf->P[0][0] - (K0 * p00);
    hkf->P[0][1] = hkf->P[0][1] - (K0 * p01);
    hkf->P[1][0] = hkf->P[1][0] - (K1 * p00);
    hkf->P[1][1] = hkf->P[1][1] - (K1 * p01);
}

/* ========================================================================== */
/* 발산 방지 (FMEA W-05)                                                      */
/* ========================================================================== */

/** @brief 공분산 발산 임계값 */
#define KF_P_MAX  10000.0f

/**
 * @brief 칼만 필터 발산 검사 및 복구
 * @param hkf 칼만 필터 핸들
 * @details 발산 조건:
 *          - 공분산 P[0][0] 또는 P[1][1] > 10000.0
 *          - 상태 변수 NaN 검출
 *          복구: 공분산 및 상태를 초기값으로 리셋
 */
void KF_CheckDivergence(KF_Handle_t *hkf) {
    /* Check if covariance has grown too large (filter divergence) */
    if ((hkf->P[0][0] > KF_P_MAX) || (hkf->P[1][1] > KF_P_MAX)) {
        #ifdef HOST_TEST_MODE
        printf("[KF] Divergence detected! P[0][0]=%.1f. Resetting covariance.\n", (double)hkf->P[0][0]);
        #endif
        
        /* Reset covariance to initial values - MISRA: separate statements */
        hkf->P[0][0] = 1.0f;
        hkf->P[0][1] = 0.0f;
        hkf->P[1][0] = 0.0f;
        hkf->P[1][1] = 1.0f;
    }
    
    /* Also check for NaN (numerical instability) - MISRA: use macro */
    if (KF_ISNAN(hkf->x[0]) || KF_ISNAN(hkf->x[1])) {
        #ifdef HOST_TEST_MODE
        printf("[KF] NaN detected! Resetting state.\n");
        #endif
        hkf->x[0] = 0.0f;
        hkf->x[1] = 0.0f;
        hkf->P[0][0] = 1.0f;
        hkf->P[0][1] = 0.0f;
        hkf->P[1][0] = 0.0f;
        hkf->P[1][1] = 1.0f;
    }
}
