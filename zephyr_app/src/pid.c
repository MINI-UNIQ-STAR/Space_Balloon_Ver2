/**
 * @file pid.c
 * @brief PID 제어기 (순수 C, 플랫폼 독립)
 */

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float setpoint;
    float integral;
    float prev_error;
    float integral_limit;
    float output_limit;
} PID_Controller_t;

static PID_Controller_t pid = {
    .kp = 1.0f,
    .ki = 0.1f,
    .kd = 0.01f,
    .setpoint = 0.0f,
    .integral = 0.0f,
    .prev_error = 0.0f,
    .integral_limit = 100.0f,
    .output_limit = 100.0f
};

void PID_Init(float kp, float ki, float kd) {
    pid.kp = kp;
    pid.ki = ki;
    pid.kd = kd;
    pid.integral = 0.0f;
    pid.prev_error = 0.0f;
}

void PID_SetSetpoint(float setpoint) {
    pid.setpoint = setpoint;
}

float PID_Compute(float measurement, float dt) {
    float error = pid.setpoint - measurement;
    
    /* 적분 */
    pid.integral += error * dt;
    if (pid.integral > pid.integral_limit) {
        pid.integral = pid.integral_limit;
    } else if (pid.integral < -pid.integral_limit) {
        pid.integral = -pid.integral_limit;
    }
    
    /* 미분 */
    float derivative = (error - pid.prev_error) / dt;
    pid.prev_error = error;
    
    /* PID 출력 */
    float output = pid.kp * error + pid.ki * pid.integral + pid.kd * derivative;
    
    /* 출력 제한 */
    if (output > pid.output_limit) {
        output = pid.output_limit;
    } else if (output < -pid.output_limit) {
        output = -pid.output_limit;
    }
    
    return output;
}

float PID_GetOutput(void) {
    return pid.kp * pid.prev_error + pid.ki * pid.integral;
}
