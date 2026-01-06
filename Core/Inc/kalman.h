#ifndef __KALMAN_H
#define __KALMAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Simple 1D Kalman Filter for Altitude (can be expanded)
typedef struct {
    // State Vector [Altitude, Vertical Velocity]
    float x[2]; 
    
    // Covariance Matrix P [2x2]
    float P[2][2];
    
    // Process Noise Covariance Q [2x2]
    float Q[2][2];
    
    // Measurement Noise Covariance R (Scalar for 1D measurement)
    float R;
    
    // Time step
    float dt;
} KF_Handle_t;

void KF_Init(KF_Handle_t *hkf, float dt, float process_noise, float meas_noise);
void KF_Predict(KF_Handle_t *hkf);
void KF_Update_Altitude(KF_Handle_t *hkf, float measurement);
void KF_CheckDivergence(KF_Handle_t *hkf);  // Reset covariance if diverged
// Placeholder for Attitude Update
// void KF_Update_Attitude(KF_Handle_t *hkf, ...);

#ifdef __cplusplus
}
#endif

#endif /* __KALMAN_H */
