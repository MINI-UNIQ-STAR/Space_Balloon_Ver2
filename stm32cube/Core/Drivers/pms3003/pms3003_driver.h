#ifndef PMS3003_DRIVER_H
#define PMS3003_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t PM_SP_UG_1_0;
    uint16_t PM_SP_UG_2_5;
    uint16_t PM_SP_UG_10_0;
    uint16_t PM_AE_UG_1_0;
    uint16_t PM_AE_UG_2_5;
    uint16_t PM_AE_UG_10_0;
} PMS_Data_t;

typedef int32_t (*pms_write_ptr)(void *, uint8_t *buf, uint16_t len);

typedef struct {
    pms_write_ptr write;
    void *handle;
    
    // Parsing State
    uint8_t _index;
    uint16_t _frameLen;
    uint16_t _checksum;
    uint16_t _calcChecksum;
    uint8_t _payload[12];
    PMS_Data_t data;
} pms_ctx_t;

void PMS_Init(pms_ctx_t *ctx);
void PMS_WakeUp(pms_ctx_t *ctx);
void PMS_Sleep(pms_ctx_t *ctx);
void PMS_ActiveMode(pms_ctx_t *ctx);
void PMS_PassiveMode(pms_ctx_t *ctx);
void PMS_RequestRead(pms_ctx_t *ctx);

// State machine to process one byte at a time (called from ISR or polling loop)
// Returns true if a full packet was parsed successfully
bool PMS_ProcessByte(pms_ctx_t *ctx, uint8_t ch);

#endif
