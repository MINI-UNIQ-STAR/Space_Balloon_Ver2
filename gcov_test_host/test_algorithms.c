/**
 * @file test_algorithms.c
 * @brief Simple Host Test for PID and Kalman algorithms
 * @details Tests pure algorithm modules without HAL dependencies
 */

#include <stdio.h>
#include <math.h>

/* Include algorithm headers directly */
#include "../Core/Inc/bsp.h"
#include "../Core/Inc/pid.h"
#include "../Core/Inc/kalman.h"

/* Test PID Controller */
void test_pid(void) {
    printf("\n=== PID Controller Test ===\n");
    
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 100.0f, 1.0f, 0.0f, 100.0f);
    hpid.Target = 25.0f;  /* Target 25°C */
    
    float temp = 10.0f;  /* Start at 10°C */
    float dt = 0.02f;    /* 50Hz */
    
    printf("Target: %.1f°C, Initial: %.1f°C\n", hpid.Target, temp);
    
    /* Simulate 10 steps */
    for (int i = 0; i < 10; i++) {
        float output = PID_Update(&hpid, temp, dt);
        temp += output * 0.01f;  /* Simulate heating */
        printf("Step %2d: Temp=%.2f°C, Output=%.2f%%\n", i+1, temp, output);
    }
    
    printf("PID Test: %s\n", (temp > 15.0f) ? "PASS" : "FAIL");
}

/* Test Kalman Filter */
void test_kalman(void) {
    printf("\n=== Kalman Filter Test ===\n");
    
    KF_Handle_t hkf;
    KF_Init(&hkf, 0.02f, 0.5f, 0.3f);  /* 50Hz, Q=0.5, R=0.3 */
    
    /* Simulate noisy altitude measurements */
    float true_alt = 100.0f;
    float measurements[] = {98.5f, 101.2f, 99.8f, 100.5f, 102.1f, 99.0f, 100.8f, 101.5f, 99.2f, 100.0f};
    
    printf("True Altitude: %.1f m\n", true_alt);
    
    for (int i = 0; i < 10; i++) {
        KF_Predict(&hkf);
        KF_Update_Altitude(&hkf, measurements[i]);
        printf("Meas %2d: %.1f m -> Filtered: %.2f m\n", i+1, measurements[i], hkf.x[0]);
    }
    
    /* Check convergence */
    float error = fabsf(hkf.x[0] - true_alt);
    printf("Final Error: %.2f m\n", error);
    printf("Kalman Test: %s\n", (error < 2.0f) ? "PASS" : "FAIL");
    
    /* Test divergence detection */
    printf("\n--- Divergence Test ---\n");
    hkf.P[0][0] = 20000.0f;  /* Force divergence */
    KF_CheckDivergence(&hkf);
    printf("After divergence check: P[0][0] = %.1f (should be 1.0)\n", hkf.P[0][0]);
    printf("Divergence Reset: %s\n", (hkf.P[0][0] == 1.0f) ? "PASS" : "FAIL");
}

int main(void) {
    printf("========================================\n");
    printf("  SpaceBalloon Algorithm Test Suite\n");
    printf("========================================\n");
    
    test_pid();
    test_kalman();
    
    printf("\n========================================\n");
    printf("  All Tests Complete!\n");
    printf("========================================\n");
    
    return 0;
}
