#ifndef XA1110_DRIVER_H
#define XA1110_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// GPS Data Structure
typedef struct {
    int32_t lat_deg_e7; // Latitude in degrees * 10^7
    int32_t lon_deg_e7; // Longitude in degrees * 10^7
    float alt_m;
    uint8_t fix_type;   // 0=No fix, 1=2D, 2=3D (Simplified)
    uint8_t sats_used;
    uint8_t sats_view_total;
    
    // Sat counts per system (Mock/Parsed if GSA/GSV available)
    uint8_t sats_gps;
    uint8_t sats_glonass;
    uint8_t sats_galileo;
    uint8_t sats_beidou;
} xa1110_data_t;

typedef int32_t (*xa1110_write_ptr)(void *, uint8_t *, uint16_t);

typedef struct {
    void *handle;
    xa1110_write_ptr write;
    uint8_t *rx_buffer;
    uint16_t rx_len;
    uint16_t rx_idx;
    xa1110_data_t data;
} xa1110_ctx_t;

void XA1110_Init(xa1110_ctx_t *ctx);
void XA1110_ProcessByte(xa1110_ctx_t *ctx, uint8_t byte);
bool XA1110_ParseSentence(xa1110_ctx_t *ctx, char *sentence);

#endif
