#include "services/ozone_service.h"
#include "drivers/sen0321.h"
#include "drivers/reset_lines.h"
#include "main.h" // For hi2c3
#include <stddef.h>

static sen0321_t s_dev;
static int16_t s_last_ppb = 0;
static bool s_has_data = false;
static uint32_t s_last_read_ms = 0;

extern I2C_HandleTypeDef hi2c3;

void ozone_service_init(void) {
    // Reset sequence: LOW -> HIGH (1ms) -> LOW
    reset_line_pulse(RESET_LINE_SEN_RST, 0, 1, 0);
    
    sen0321_init(&s_dev, &hi2c3);
    s_last_read_ms = 0;
}

void ozone_service_tick(uint32_t now_ms) {
    // Read every 1000ms (1Hz)
    if (now_ms - s_last_read_ms < 1000) {
        return;
    }

    int16_t ppb;
    if (sen0321_read_ppb(&s_dev, &ppb)) {
        s_last_ppb = ppb;
        s_has_data = true;
        s_last_read_ms = now_ms;
    }
}

bool ozone_service_get_ppb(int16_t *out_ppb) {
    if (!s_has_data) {
        return false;
    }
    *out_ppb = s_last_ppb;
    return true;
}

bool ozone_service_get_last_update_ms(uint32_t *out_ms) {
    if (!s_has_data || out_ms == NULL) {
        return false;
    }
    *out_ms = s_last_read_ms;
    return true;
}

void ozone_service_reset(void) {
    s_has_data = false;
    s_last_read_ms = 0;
    s_last_ppb = 0;
    // Re-init sensor
    reset_line_pulse(RESET_LINE_SEN_RST, 0, 1, 0);
    sen0321_init(&s_dev, &hi2c3);
}
