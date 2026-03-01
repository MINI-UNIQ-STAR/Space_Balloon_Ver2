#ifndef __PID_H
#define __PID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float MaxOutput;
    
    float Target;
    float IntegratedError;
    float LastError;
} PID_HandleTypeDef;

void PID_Init(PID_HandleTypeDef *hpid, float Kp, float Ki, float Kd, float MaxOutput);
float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H */
