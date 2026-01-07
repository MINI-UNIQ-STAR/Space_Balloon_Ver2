#include "pid.h"

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
    
    /* Integral term */
    hpid->IntegratedError += error * dt;
    float i_term = hpid->Ki * hpid->IntegratedError;
    
    /* Derivative term */
    float derivative = (error - hpid->LastError) / dt;
    float d_term = hpid->Kd * derivative;
    
    hpid->LastError = error;
    
    float output = p_term + i_term + d_term;
    
    /* Clamping (Anti-windup simple) */
    if (output > hpid->MaxOutput) {
        output = hpid->MaxOutput;
    } else if (output < -hpid->MaxOutput) {
        output = -hpid->MaxOutput; /* Assuming symmetric limits or 0 lower bound depending on application */
        /* If output is strictly positive (like heater PWM 0-100%), clamp to 0. */
        /* Let's assume heater is 0 to Max. */
        if (output < 0.0f) {
            output = 0.0f;
        }
    }
    
    return output;
}
