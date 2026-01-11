#include "pid.h"
#include <stdbool.h>
void PID_Init(PID_HandleTypeDef *hpid, float Kp, float Ki, float Kd, float MaxOutput) {
    hpid->Kp = Kp;
    hpid->Ki = Ki;
    hpid->Kd = Kd;
    hpid->MaxOutput = MaxOutput;
    
    hpid->Target = 0.0f;
    hpid->IntegratedError = 0.0f;
    hpid->LastError = 0.0f;
}

float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt) {
    float error = hpid->Target - measurement;
    
    /* Proportional term */
    float p_term = hpid->Kp * error;
    
    /* Derivative term */
    float derivative = (error - hpid->LastError) / dt;
    float d_term = hpid->Kd * derivative;
    hpid->LastError = error;
    
    /* Tentative output without new I-term contribution */
    float tentative_output = p_term + (hpid->Ki * hpid->IntegratedError) + d_term;
    
    /* Conditional Integration (Anti-windup):
     * Only accumulate I-term if output is not saturated.
     * Saturated means: output >= MaxOutput OR output <= 0 when error pushes further */
    bool saturated_high = (tentative_output >= hpid->MaxOutput) && (error > 0.0f);
    bool saturated_low = (tentative_output <= 0.0f) && (error < 0.0f);
    
    if (!saturated_high && !saturated_low) {
        hpid->IntegratedError += error * dt;
    }
    
    float i_term = hpid->Ki * hpid->IntegratedError;
    float output = p_term + i_term + d_term;
    
    /* Output Clamping */
    if (output > hpid->MaxOutput) {
        output = hpid->MaxOutput;
    } else if (output < 0.0f) {
        output = 0.0f;
    }
    
    return output;
}
