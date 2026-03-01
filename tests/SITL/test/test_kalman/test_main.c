/**
 * @file test_main.c
 * @brief 칼만 필터(Kalman Filter) 알고리즘 단위 테스트 (SITL)
 * @details 1차원 칼만 필터(고도, 수직 속도 추정)의 수학적 정확성 검증
 *          - 초기화 및 공분산 행렬(P) 설정 확인
 *          - 상태 예측(Predict) 모델 정확성 테스트
 *          - 측정 업데이트(Update) 후 값 수렴성 및 노이즈 제거 성능 확인
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include <stdio.h>
#include "unity.h"
#include "kalman.h"

// Defined in kalman.c
extern void KF_Init(KF_Handle_t *hkf, float dt, float process_noise, float meas_noise);
extern void KF_Predict(KF_Handle_t *hkf);
extern void KF_Update_Altitude(KF_Handle_t *hkf, float measurement);

void setUp(void) {
}

void tearDown(void) {
}

/** @brief 칼만 필터 초기화 및 파라미터 설정 검증 */
void test_kf_init(void) {
    KF_Handle_t hkf;
    float dt = 0.02f; // 50Hz
    float Q = 0.5f;
    float R = 0.3f;
    
    KF_Init(&hkf, dt, Q, R);
    
    // 설정 값 확인
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, dt, hkf.dt);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, R, hkf.R);
    
    // 초기 상태 확인 (0으로 초기화됨)
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hkf.x[0]); // 고도
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hkf.x[1]); // 속도
    
    // 초기 공분산 행렬 확인 (대각 성분 1.0)
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hkf.P[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hkf.P[1][1]);
}

/** @brief 상태 예측(Prediction) 단계 테스트 */
void test_kf_predict(void) {
    KF_Handle_t hkf;
    KF_Init(&hkf, 1.0f, 0.0f, 0.1f); // dt=1.0s (계산 단순화), Q=0 (결정론적)
    
    // 초기 상태 설정: 고도=100m, 속도=5m/s
    hkf.x[0] = 100.0f;
    hkf.x[1] = 5.0f;
    
    KF_Predict(&hkf);
    
    // 1초 후 예상 상태:
    // 고도 = 100 + 5*1 = 105
    // 속도 = 5 (등속 모델)
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 105.0f, hkf.x[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, hkf.x[1]);
}

/** @brief 측정 업데이트 및 수렴성 테스트 */
void test_kf_update_convergence(void) {
    KF_Handle_t hkf;
    KF_Init(&hkf, 0.1f, 0.1f, 0.5f); // dt=0.1s
    
    // 초기 상태: 0m
    
    // 측정값이 지속적으로 10m라고 할 때 필터가 수렴하는지 확인
    // 50회 반복 (5초)
    for(int i=0; i<50; i++) {
        KF_Predict(&hkf);
        KF_Update_Altitude(&hkf, 10.0f);
    }
    
    // 고도가 10.0에 근접해야 함
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, hkf.x[0]);
    
    // 속도는 0에 근접해야 함 (정지 상태 10m)
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, hkf.x[1]);
}

/** @brief 상승 프로파일(Ascent Profile)에 대한 추적 성능 테스트 */
void test_kf_ascent_profile(void) {
    KF_Handle_t hkf;
    KF_Init(&hkf, 0.1f, 0.5f, 5.0f); // 노이즈 있는 센서 가정 (R=5)
    
    // 5m/s 상승 시뮬레이션
    float true_alt = 0.0f;
    float vel = 5.0f;
    float dt = 0.1f;
    
    for(int i=0; i<100; i++) {
        true_alt += vel * dt;
        float noisy_meas = true_alt; /* 테스트 단순화를 위해 이상적인 센서 값 사용 (Lag 확인용) */
        
        KF_Predict(&hkf);
        KF_Update_Altitude(&hkf, noisy_meas);
    }
    
    // 필터가 위치를 잘 추적하는지 확인
    TEST_ASSERT_FLOAT_WITHIN(2.0f, true_alt, hkf.x[0]);
    // 필터가 속도를 약 5m/s로 추정하는지 확인
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 5.0f, hkf.x[1]);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_kf_init);
    RUN_TEST(test_kf_predict);
    RUN_TEST(test_kf_update_convergence);
    RUN_TEST(test_kf_ascent_profile);
    
    return UNITY_END();
}
