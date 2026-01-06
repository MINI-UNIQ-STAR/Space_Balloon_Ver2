#include "kalman.h"

#ifdef HOST_TEST_MODE
#include <stdio.h>
#endif

void KF_Init(KF_Handle_t *hkf, float dt, float process_noise, float meas_noise) {
    hkf->dt = dt;
    
    // Initial State
    hkf->x[0] = 0.0f; // Altitude
    hkf->x[1] = 0.0f; // Velocity
    
    // Initial Covariance
    hkf->P[0][0] = 1.0f; hkf->P[0][1] = 0.0f;
    hkf->P[1][0] = 0.0f; hkf->P[1][1] = 1.0f;
    
    // Process Noise Q (Constant Velocity Model assumption)
    // Q = q * [dt^3/3  dt^2/2; dt^2/2  dt]
    // Simplified for now
    hkf->Q[0][0] = process_noise; hkf->Q[0][1] = 0.0f;
    hkf->Q[1][0] = 0.0f;          hkf->Q[1][1] = process_noise;
    
    hkf->R = meas_noise;
}

void KF_Predict(KF_Handle_t *hkf) {
    // F = [1 dt; 0 1]
    float old_alt = hkf->x[0];
    float old_vel = hkf->x[1];
    
    // State Prediction: x = F * x
    hkf->x[0] = old_alt + old_vel * hkf->dt;
    hkf->x[1] = old_vel;
    
    // Covariance Prediction: P = F * P * F^T + Q
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

void KF_Update_Altitude(KF_Handle_t *hkf, float measurement) {
    // Run Prediction First
    KF_Predict(hkf);
    
    // H = [1 0]
    // y = z - H * x
    float y = measurement - hkf->x[0];
    
    // S = H * P * H^T + R
    float S = hkf->P[0][0] + hkf->R;
    
    // K = P * H^T * S^-1
    float K0 = hkf->P[0][0] / S;
    float K1 = hkf->P[1][0] / S;
    
    // Update State: x = x + K * y
    hkf->x[0] += K0 * y;
    hkf->x[1] += K1 * y;
    
    // Update Covariance: P = (I - K * H) * P
    float p00 = hkf->P[0][0];
    float p01 = hkf->P[0][1];
    
    hkf->P[0][0] -= K0 * p00;
    hkf->P[0][1] -= K0 * p01;
    hkf->P[1][0] -= K1 * p00;
    hkf->P[1][1] -= K1 * p01;
}

// ===== Divergence Protection (FMEA W-05) =====
#define KF_P_MAX  10000.0f  // Covariance divergence threshold

void KF_CheckDivergence(KF_Handle_t *hkf) {
    // Check if covariance has grown too large (filter divergence)
    if (hkf->P[0][0] > KF_P_MAX || hkf->P[1][1] > KF_P_MAX) {
        #ifdef HOST_TEST_MODE
        printf("[KF] Divergence detected! P[0][0]=%.1f. Resetting covariance.\n", hkf->P[0][0]);
        #endif
        
        // Reset covariance to initial values
        hkf->P[0][0] = 1.0f; hkf->P[0][1] = 0.0f;
        hkf->P[1][0] = 0.0f; hkf->P[1][1] = 1.0f;
    }
    
    // Also check for NaN (numerical instability)
    if (hkf->x[0] != hkf->x[0] || hkf->x[1] != hkf->x[1]) { // NaN check
        #ifdef HOST_TEST_MODE
        printf("[KF] NaN detected! Resetting state.\n");
        #endif
        hkf->x[0] = 0.0f;
        hkf->x[1] = 0.0f;
        hkf->P[0][0] = 1.0f; hkf->P[0][1] = 0.0f;
        hkf->P[1][0] = 0.0f; hkf->P[1][1] = 1.0f;
    }
}
