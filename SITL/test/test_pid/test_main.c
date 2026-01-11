#include <stdio.h>
#include "unity.h"
#include "pid.h"

// Defined in pid.c
// void PID_Init(PID_HandleTypeDef *hpid, float Kp, float Ki, float Kd, float MaxOutput);
// float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt);

void setUp(void) {
}

void tearDown(void) {
}

void test_pid_init(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 1.0f, 2.0f, 3.0f, 100.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hpid.Kp);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, hpid.Ki);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, hpid.Kd);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, hpid.MaxOutput);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hpid.IntegratedError);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hpid.LastError);
}

// Test Pure Proportional Component
void test_pid_p_term(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 2.0f, 0.0f, 0.0f, 100.0f); // Kp=2
    
    hpid.Target = 10.0f;
    float meas = 5.0f; // Error = 5
    float dt = 1.0f;
    
    // Output = Kp * Error = 2 * 5 = 10
    float output = PID_Update(&hpid, meas, dt);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, output);
}

// Test Integral Term Accumulation
void test_pid_i_term(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 0.0f, 1.0f, 0.0f, 100.0f); // Ki=1
    hpid.Target = 10.0f;
    
    // Step 1: Error = 2, dt = 1 -> IntError += 2
    PID_Update(&hpid, 8.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, hpid.IntegratedError); 
    
    // Step 2: Error = 2, dt = 1 -> IntError += 2 -> Total 4
    float output = PID_Update(&hpid, 8.0f, 1.0f);
    
    // Output = Ki * IntError = 1 * 4 = 4
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, output);
}

// Test Derivative Term
void test_pid_d_term(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 0.0f, 0.0f, 1.0f, 100.0f); // Kd=1
    hpid.Target = 10.0f;
    
    // Step 1: Meas=5, Error=5, LastError=0 (Init)
    // Derivative = (5-0)/1 = 5
    // Output = 1 * 5 = 5
    float out1 = PID_Update(&hpid, 5.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, out1);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, hpid.LastError);
    
    // Step 2: Meas=8, Error=2
    // Derivative = (2 - 5) / 1 = -3
    // Output = 1 * -3 = -3 -> Clamped to 0 (heater can't go negative)
    float out2 = PID_Update(&hpid, 8.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, out2);
}

// Test Output Clamping (0 to Max)
void test_pid_clamping(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 10.0f, 0.0f, 0.0f, 50.0f); // Max 50
    hpid.Target = 100.0f;
    
    // Case 1: High positive error
    // Error = 100, Output = 1000 -> Clamp to 50
    float out1 = PID_Update(&hpid, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 50.0f, out1);
    
    // Case 2: Negative error
    // Error = -50 (Meas 150), Output = -500 -> Clamp to 0 (Heater logic)
    float out2 = PID_Update(&hpid, 150.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, out2);
}

// Test Anti-windup: I-term should NOT accumulate when output is saturated
void test_pid_anti_windup(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 100.0f, 1.0f, 0.0f, 50.0f); // Kp=100, Max=50
    hpid.Target = 100.0f;
    
    // Step 1: Meas=0 -> Error=100 -> P=10000 -> Saturated at 50!
    // Output tentative = 10000 > Max, so I-term should NOT accumulate
    PID_Update(&hpid, 0.0f, 1.0f);
    float ie1 = hpid.IntegratedError;
    
    // Step 2: Still saturated, I-term should still NOT accumulate
    PID_Update(&hpid, 0.0f, 1.0f);
    float ie2 = hpid.IntegratedError;
    
    // Anti-windup verification: I-term should not have grown
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, ie1, ie2);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, ie1); // Should be 0
}

int main(void) {
    UnityBegin();
    
    RUN_TEST(test_pid_init);
    RUN_TEST(test_pid_p_term);
    RUN_TEST(test_pid_i_term);
    RUN_TEST(test_pid_d_term);
    RUN_TEST(test_pid_clamping);
    RUN_TEST(test_pid_anti_windup);
    
    return UnityEnd();
}
