/**
 * @file kalman.c
 * @brief 칼만 필터 (순수 C, 플랫폼 독립)
 */

#include <stdint.h>
#include <math.h>

typedef struct {
    float q;  /* 프로세스 노이즈 분산 */
    float r;  /* 측정 노이즈 분산 */
    float x;  /* 추정값 */
    float p;  /* 추정 오차 분산 */
    float k;  /* 칼만 이득 */
} KalmanFilter_t;

static KalmanFilter_t altitude_filter = {
    .q = 0.01f,
    .r = 0.1f,
    .x = 0.0f,
    .p = 1.0f,
    .k = 0.0f
};

void Kalman_Init(float initial_value) {
    altitude_filter.x = initial_value;
    altitude_filter.p = 1.0f;
}

float Kalman_Update(float measurement) {
    /* 예측 */
    altitude_filter.p = altitude_filter.p + altitude_filter.q;
    
    /* 칼만 이득 계산 */
    altitude_filter.k = altitude_filter.p / (altitude_filter.p + altitude_filter.r);
    
    /* 추정값 갱신 */
    altitude_filter.x = altitude_filter.x + altitude_filter.k * (measurement - altitude_filter.x);
    
    /* 오차 분산 갱신 */
    altitude_filter.p = (1.0f - altitude_filter.k) * altitude_filter.p;
    
    return altitude_filter.x;
}

float Kalman_GetEstimate(void) {
    return altitude_filter.x;
}
