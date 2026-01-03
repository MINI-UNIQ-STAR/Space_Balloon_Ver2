#include "services/heater_service.h"
#include "services/aux_sensors_service.h"
#include "main.h" // For TIM16, TIM3 handles

#include <math.h>

extern TIM_HandleTypeDef htim16; // Battery Heater (PA6)
extern TIM_HandleTypeDef htim3;  // Board Heater (PB1)

// PID Constants (Tuning required!)
// Battery Heater (Kapton)
#define BAT_HEATER_KP 1000.0f
#define BAT_HEATER_KI 10.0f
#define BAT_HEATER_KD 0.0f
#define BAT_HEATER_INTEGRAL_MAX 30000.0f

// Board Heater (Minibulb)
#define BOARD_HEATER_KP 500.0f
#define BOARD_HEATER_KI 5.0f
#define BOARD_HEATER_KD 0.0f
#define BOARD_HEATER_INTEGRAL_MAX 15000.0f

#define HEATER_PWM_PERIOD 65535.0f

typedef struct {
    float target_temp_c;
    float integral_err;
    float last_error;
    float current_duty;
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    // PID Parameters
    float kp;
    float ki;
    float kd;
    float integral_max;
} heater_ctrl_t;

static heater_ctrl_t s_bat_heater;
static heater_ctrl_t s_board_heater;
static uint32_t s_last_tick_ms = 0;

static void heater_ctrl_init(heater_ctrl_t *h, TIM_HandleTypeDef *htim, uint32_t channel, float default_target,
                             float kp, float ki, float kd, float integral_max) {
    h->htim = htim;
    h->channel = channel;
    h->target_temp_c = default_target;
    h->integral_err = 0.0f;
    h->last_error = 0.0f;
    h->current_duty = 0.0f;
    h->kp = kp;
    h->ki = ki;
    h->kd = kd;
    h->integral_max = integral_max;
    
    HAL_TIM_PWM_Start(h->htim, h->channel);
    __HAL_TIM_SET_COMPARE(h->htim, h->channel, 0);
}

static void heater_ctrl_update(heater_ctrl_t *h, float current_temp_c, float dt) {
    float error = h->target_temp_c - current_temp_c;

    // Proportional
    float p_term = h->kp * error;

    // Integral
    h->integral_err += error * dt;
    // Anti-windup
    if (h->integral_err > h->integral_max) h->integral_err = h->integral_max;
    if (h->integral_err < -h->integral_max) h->integral_err = -h->integral_max;
    float i_term = h->ki * h->integral_err;

    // Derivative
    float d_term = h->kd * (error - h->last_error) / dt;
    h->last_error = error;

    // Output
    float output = p_term + i_term + d_term;

    // Clamp output to PWM range
    if (output > HEATER_PWM_PERIOD) output = HEATER_PWM_PERIOD;
    if (output < 0.0f) output = 0.0f;

    // Apply to PWM
    __HAL_TIM_SET_COMPARE(h->htim, h->channel, (uint32_t)output);
    h->current_duty = output / HEATER_PWM_PERIOD;
}

static void heater_ctrl_off(heater_ctrl_t *h) {
    __HAL_TIM_SET_COMPARE(h->htim, h->channel, 0);
    h->current_duty = 0.0f;
    h->integral_err = 0.0f;
}

void heater_service_init(void)
{
    heater_ctrl_init(&s_bat_heater, &htim16, TIM_CHANNEL_1, 10.0f, 
                     BAT_HEATER_KP, BAT_HEATER_KI, BAT_HEATER_KD, BAT_HEATER_INTEGRAL_MAX);
    
    heater_ctrl_init(&s_board_heater, &htim3, TIM_CHANNEL_4, 5.0f,
                     BOARD_HEATER_KP, BOARD_HEATER_KI, BOARD_HEATER_KD, BOARD_HEATER_INTEGRAL_MAX);
    
    s_last_tick_ms = HAL_GetTick();
}

void heater_service_tick(uint32_t now_ms)
{
    // Run PID loop at 1Hz (aligned with sensor updates)
    if (now_ms - s_last_tick_ms < 1000) {
        return;
    }
    
    float dt = (now_ms - s_last_tick_ms) / 1000.0f;
    s_last_tick_ms = now_ms;

    int16_t temp_x100;

    // 1. Battery Heater
    if (aux_sensors_get_bat_temp_c_x100(&temp_x100)) {
        heater_ctrl_update(&s_bat_heater, temp_x100 / 100.0f, dt);
    } else {
        heater_ctrl_off(&s_bat_heater);
    }

    // 2. Board Heater
    if (aux_sensors_get_board_temp_c_x100(&temp_x100)) {
        heater_ctrl_update(&s_board_heater, temp_x100 / 100.0f, dt);
    } else {
        heater_ctrl_off(&s_board_heater);
    }
}

void heater_bat_set_target(float temp_c) { s_bat_heater.target_temp_c = temp_c; }
float heater_bat_get_target(void) { return s_bat_heater.target_temp_c; }
float heater_bat_get_duty(void) { return s_bat_heater.current_duty; }

void heater_board_set_target(float temp_c) { s_board_heater.target_temp_c = temp_c; }
float heater_board_get_target(void) { return s_board_heater.target_temp_c; }
float heater_board_get_duty(void) { return s_board_heater.current_duty; }
