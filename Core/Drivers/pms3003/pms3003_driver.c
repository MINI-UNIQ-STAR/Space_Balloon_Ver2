#include "pms3003_driver.h"
#include <string.h> // for memset

// Endianness helper
static uint16_t makeWord(uint8_t h, uint8_t l) {
    return (h << 8) | l;
}

void PMS_Init(pms_ctx_t *ctx) {
    ctx->_index = 0;
    ctx->_frameLen = 0;
    ctx->_checksum = 0;
    ctx->_calcChecksum = 0;
    memset(ctx->_payload, 0, sizeof(ctx->_payload));
    memset(&ctx->data, 0, sizeof(PMS_Data_t));
}

void PMS_WakeUp(pms_ctx_t *ctx) {
    uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74 };
    ctx->write(ctx->handle, command, sizeof(command));
}

void PMS_Sleep(pms_ctx_t *ctx) {
    uint8_t command[] = { 0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73 };
    ctx->write(ctx->handle, command, sizeof(command));
}

void PMS_ActiveMode(pms_ctx_t *ctx) {
    uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
    ctx->write(ctx->handle, command, sizeof(command));
}

void PMS_PassiveMode(pms_ctx_t *ctx) {
    uint8_t command[] = { 0x42, 0x4D, 0xE1, 0x00, 0x00, 0x01, 0x70 };
    ctx->write(ctx->handle, command, sizeof(command));
}

void PMS_RequestRead(pms_ctx_t *ctx) {
    uint8_t command[] = { 0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71 };
    ctx->write(ctx->handle, command, sizeof(command));
}

bool PMS_ProcessByte(pms_ctx_t *ctx, uint8_t ch) {
    switch (ctx->_index) {
        case 0:
            if (ch != 0x42) return false;
            ctx->_calcChecksum = ch;
            break;
        case 1:
            if (ch != 0x4D) {
                ctx->_index = 0;
                return false;
            }
            ctx->_calcChecksum += ch;
            break;
        case 2:
            ctx->_calcChecksum += ch;
            ctx->_frameLen = ch << 8;
            break;
        case 3:
            ctx->_frameLen |= ch;
            ctx->_calcChecksum += ch;
            break;
        default:
            if (ctx->_index == ctx->_frameLen + 2) {
                ctx->_checksum = ch << 8;
            } else if (ctx->_index == ctx->_frameLen + 2 + 1) {
                ctx->_checksum |= ch;
                
                if (ctx->_calcChecksum == ctx->_checksum) {
                    // Standard Particles, CF=1
                    ctx->data.PM_SP_UG_1_0 = makeWord(ctx->_payload[0], ctx->_payload[1]);
                    ctx->data.PM_SP_UG_2_5 = makeWord(ctx->_payload[2], ctx->_payload[3]);
                    ctx->data.PM_SP_UG_10_0 = makeWord(ctx->_payload[4], ctx->_payload[5]);
                    
                    // Atmospheric Environment
                    ctx->data.PM_AE_UG_1_0 = makeWord(ctx->_payload[6], ctx->_payload[7]);
                    ctx->data.PM_AE_UG_2_5 = makeWord(ctx->_payload[8], ctx->_payload[9]);
                    ctx->data.PM_AE_UG_10_0 = makeWord(ctx->_payload[10], ctx->_payload[11]);
                    
                    ctx->_index = 0;
                    return true;
                }
                ctx->_index = 0;
                return false;
            } else {
                ctx->_calcChecksum += ch;
                uint8_t payloadIndex = ctx->_index - 4;
                if (payloadIndex < sizeof(ctx->_payload)) {
                    ctx->_payload[payloadIndex] = ch;
                }
            }
            break;
    }
    ctx->_index++;
    return false;
}
