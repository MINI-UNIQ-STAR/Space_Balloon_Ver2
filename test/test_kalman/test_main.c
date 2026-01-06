#include <stdio.h>
#include "unity.h"
#include "kalman.h"

// Defined in kalman.c
extern void KF_Init(KF_Handle_t *hkf, float dt, float process_noise, float meas_noise);
extern void KF_Predict(KF_Handle_t *hkf);
extern void KF_Update_Altitude(KF_Handle_t *hkf, float measurement);

void test_kf_init(void) {
    KF_Handle_t hkf;
    float dt = 0.02f; // 50Hz
    float Q = 0.5f;
    float R = 0.3f;
    
    KF_Init(&hkf, dt, Q, R);
    
    // Check Config
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, dt, hkf.dt);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, R, hkf.R);
    
    // Check Initial State
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hkf.x[0]); // Alt
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hkf.x[1]); // Vel
    
    // Check Initial Covariance (Diagonal 1.0)
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hkf.P[0][0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hkf.P[1][1]);
}

void test_kf_predict(void) {
    KF_Handle_t hkf;
    KF_Init(&hkf, 1.0f, 0.0f, 0.1f); // dt=1.0s to make math easy, Q=0 for determinstic
    
    // Set initial state: Alt=100m, Vel=5m/s
    hkf.x[0] = 100.0f;
    hkf.x[1] = 5.0f;
    
    KF_Predict(&hkf);
    
    // After 1 sec, Alt should be 100 + 5*1 = 105
    // Vel should be same 5
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 105.0f, hkf.x[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, hkf.x[1]);
}

void test_kf_update_convergence(void) {
    KF_Handle_t hkf;
    KF_Init(&hkf, 0.1f, 0.1f, 0.5f); // dt=0.1s
    
    // Initial: 0m
    
    // Measurements say 10m. Filter should move towards 10m.
    // Loop 50 times (5 seconds)
    for(int i=0; i<50; i++) {
        KF_Update_Altitude(&hkf, 10.0f);
    }
    
    // Should be very close to 10.0
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, hkf.x[0]);
    
    // Velocity should be near 0 (static 10m)
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, hkf.x[1]);
}

// Test response to generic Ascent profile
void test_kf_ascent_profile(void) {
    KF_Handle_t hkf;
    KF_Init(&hkf, 0.1f, 0.5f, 5.0f); // noisy sensor (R=5)
    
    // Simulating 5m/s ascent
    float true_alt = 0.0f;
    float vel = 5.0f;
    float dt = 0.1f;
    
    for(int i=0; i<100; i++) {
        true_alt += vel * dt;
        float noisy_meas = true_alt; // Perfect sensor for this simple test check lag
        
        KF_Update_Altitude(&hkf, noisy_meas);
    }
    
    // Filter tracks position well
    TEST_ASSERT_FLOAT_WITHIN(2.0f, true_alt, hkf.x[0]);
    // Filter estimates velocity approx 5m/s
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 5.0f, hkf.x[1]);
}

int main(void) {
    UnityBegin();
    
    RUN_TEST(test_kf_init);
    RUN_TEST(test_kf_predict);
    RUN_TEST(test_kf_update_convergence);
    RUN_TEST(test_kf_ascent_profile);
    
    return UnityEnd();
}
