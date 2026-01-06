#ifndef XCP_H
#define XCP_H

#include <stdint.h>
#include "pid.h"
#include "kalman.h"

// XCP Constants
#define XCP_MAX_PACKET_SIZE 64

// Exposed Variables for Calibration (Measurement)
typedef struct {
    float *ptr;
    uint8_t size;
    uint8_t type;
} XCP_ODT_Entry_t;

// Function Prototypes
void XCP_Init(void);
void XCP_ProcessCommand(uint8_t *data, uint8_t len);
void XCP_UpdateMeasurements(void);

// Link to Global Algorithms for Calibration
extern PID_HandleTypeDef hpid_bat;
extern PID_HandleTypeDef hpid_brd;
extern KF_Handle_t hkf;

#endif
