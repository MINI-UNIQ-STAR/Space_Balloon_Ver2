#include "xa1110_driver.h"
#include "minmea.h"
#include <string.h>
#include <stdio.h>

#define XA1110_BUF_SIZE 256
static char line_buffer[XA1110_BUF_SIZE];
static uint16_t line_idx = 0;

static void XA1110_SendCommand(xa1110_ctx_t *ctx, const char *cmd) {
    if (ctx->write) {
        ctx->write(ctx->handle, (uint8_t*)cmd, (uint16_t)strlen(cmd));
    }
}

void XA1110_Init(xa1110_ctx_t *ctx) {
    memset(&ctx->data, 0, sizeof(xa1110_data_t));
    ctx->rx_idx = 0;
    
    // Config: 1. Set Balloon Mode (High Altitude > 18km)
    // $PMTK886,3*2B<CR><LF>
    XA1110_SendCommand(ctx, "$PMTK886,3*2B\r\n");
    
    // Config: 2. Set Update Rate to 10Hz
    // $PMTK220,100*2F<CR><LF>
    XA1110_SendCommand(ctx, "$PMTK220,100*2F\r\n");
}

void XA1110_ProcessByte(xa1110_ctx_t *ctx, uint8_t byte) {
    if (byte == '\n' || byte == '\r') {
        if (line_idx > 0) {
            line_buffer[line_idx] = '\0';
            if (XA1110_ParseSentence(ctx, line_buffer)) {
                // Good parse
            }
            line_idx = 0;
        }
    } else {
        if (line_idx < XA1110_BUF_SIZE - 1) {
            line_buffer[line_idx++] = byte;
        } else {
            // Buffer overflow, reset
            line_idx = 0;
        }
    }
}

bool XA1110_ParseSentence(xa1110_ctx_t *ctx, char *sentence) {
    switch (minmea_sentence_id(sentence, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, sentence)) {
                ctx->data.lat_deg_e7 = minmea_rescale(&frame.latitude, 10000000);
                ctx->data.lon_deg_e7 = minmea_rescale(&frame.longitude, 10000000);
                ctx->data.fix_type = frame.valid ? 2 : 0; // Simple boolean to fix type
                
                // Parse UTC Time from RMC
                ctx->data.utc_hour = frame.time.hours;
                ctx->data.utc_min = frame.time.minutes;
                ctx->data.utc_sec = frame.time.seconds;
                ctx->data.utc_year = frame.date.year + 2000; // RMC year is 2-digit
                ctx->data.utc_month = frame.date.month;
                ctx->data.utc_day = frame.date.day;
            }
        } break;
        
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, sentence)) {
                ctx->data.fix_type = frame.fix_quality;
                ctx->data.sats_used = frame.satellites_tracked;
                ctx->data.alt_m = minmea_tofloat(&frame.altitude);
                ctx->data.lat_deg_e7 = minmea_rescale(&frame.latitude, 10000000);
                ctx->data.lon_deg_e7 = minmea_rescale(&frame.longitude, 10000000);
            }
        } break;

        case MINMEA_SENTENCE_GSV: {
            struct minmea_sentence_gsv frame;
            if (minmea_parse_gsv(&frame, sentence)) {
                // Determine system by Talker ID
                char talker[3];
                if (minmea_talker_id(talker, sentence)) {
                    if (talker[0] == 'G' && talker[1] == 'P') ctx->data.sats_gps = frame.total_sats;
                    else if (talker[0] == 'G' && talker[1] == 'L') ctx->data.sats_glonass = frame.total_sats;
                    else if (talker[0] == 'G' && talker[1] == 'A') ctx->data.sats_galileo = frame.total_sats;
                    else if (talker[0] == 'G' && talker[1] == 'B') ctx->data.sats_beidou = frame.total_sats;
                }

                // Update total visible
                ctx->data.sats_view_total = ctx->data.sats_gps + ctx->data.sats_glonass +
                                          ctx->data.sats_galileo + ctx->data.sats_beidou;
            }
        } break;
        
        default:
            return false;
    }
    return true;
}
