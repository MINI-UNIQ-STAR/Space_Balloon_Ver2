#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include "can_transport.h"
#include "telemetry_deserialize.h"

LOG_MODULE_REGISTER(can_int, LOG_LEVEL_INF);

#define CAN_NODE DT_LABEL(DT_CHOSEN(zephyr_can_primary))

static const struct device *can_dev;

static void reassembled_cb(uint32_t can_id, const uint8_t *buf, size_t len, void *ctx) {
    ARG_UNUSED(ctx);
    telemetry_frame_t frame;
    size_t parsed = 0;
    int r = telemetry_deserialize(buf, len, &frame, &parsed);
    if (r == 0) {
        LOG_INF("Telem received via CAN id=0x%03X seq=%u", (unsigned)can_id, frame.seq);
        // TODO: process frame (store/log/forward)
    } else {
        LOG_ERR("Telem deserialize failed r=%d", r);
    }
}

void Can_Integration_Init(void) {
    can_dev = device_get_binding(CAN_NODE);
    if (!can_dev) {
        LOG_ERR("CAN device %s not found", CAN_NODE);
        return;
    }
    LOG_INF("CAN device %s ready", CAN_NODE);
    // initialize transport layer (bitrate placeholder)
    can_transport_init(500);
    can_transport_register_reassembled_cb(reassembled_cb, NULL);

    // Zephyr CAN reception setup: use a reception filter for all extended IDs (simple)
    struct can_filter filter = { .id = 0, .id_mask = 0, .rtr = CAN_DATAFRAME }; 
    can_attach_msgq(can_dev, NULL, 0, &filter);
    LOG_INF("CAN transport integrated");
}

// Simple poll thread to read CAN frames and feed into transport
static void can_rx_thread(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    struct can_frame frame;
    while (1) {
        int ret = can_read(can_dev, &frame, K_MSEC(100), NULL);
        if (ret == 0) {
            // forward raw 8 bytes to transport handler
            can_transport_handle_rx(frame.id, frame.data);
        }
    }
}

K_THREAD_DEFINE(can_rx_tid, 1024, can_rx_thread, NULL, NULL, NULL, 7, 0, 0);

