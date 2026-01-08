#include "xcp.h"
#include <string.h>

// XCP Command Codes
#define XCP_CMD_CONNECT         0xFF
#define XCP_CMD_DISCONNECT      0xFE
#define XCP_CMD_GET_STATUS      0xFD
#define XCP_CMD_SYNCH           0xFC
#define XCP_CMD_SET_MTA         0xF6
#define XCP_CMD_UPLOAD          0xF5
#define XCP_CMD_SHORT_UPLOAD    0xF4
#define XCP_CMD_DOWNLOAD        0xF0

// XCP Response Codes
#define XCP_RES_OK              0xFF
#define XCP_RES_ERR             0xFE

// XCP Error Codes
#define XCP_ERR_CMD_SYNCH       0x00
#define XCP_ERR_CMD_BUSY        0x10
#define XCP_ERR_UNKNOWN         0x20
#define XCP_ERR_OUT_OF_RANGE    0x22
#define XCP_ERR_ACCESS_DENIED   0x25

// Global Algorithm Instances (External linkage)
extern PID_HandleTypeDef hpid_bat;
extern PID_HandleTypeDef hpid_brd;
extern KF_Handle_t hkf;

// Command/Response Buffers
static uint8_t xcp_rx_buf[XCP_MAX_PACKET_SIZE];
static uint8_t xcp_tx_buf[XCP_MAX_PACKET_SIZE];

// Memory Transfer Address (MTA)
static uint32_t xcp_mta = 0;

// Connection State
static uint8_t xcp_connected = 0;

void XCP_Init(void) {
    // Initialize XCP Protocol Layer
    memset(xcp_rx_buf, 0, XCP_MAX_PACKET_SIZE);
    memset(xcp_tx_buf, 0, XCP_MAX_PACKET_SIZE);
    xcp_mta = 0;
    xcp_connected = 0;
}

static void XCP_SendResponse(uint8_t *response, uint8_t len) {
    // In real implementation: Send via CAN/UART
    // For now, just copy to tx buffer
    memcpy(xcp_tx_buf, response, len);
}

static void XCP_SendError(uint8_t error_code) {
    uint8_t response[2];
    response[0] = XCP_RES_ERR;
    response[1] = error_code;
    XCP_SendResponse(response, 2);
}

void XCP_ProcessCommand(uint8_t *data, uint8_t len) {
    if (len == 0) return;

    uint8_t cmd = data[0];
    uint8_t response[XCP_MAX_PACKET_SIZE];

    switch (cmd) {
        case XCP_CMD_CONNECT: {
            // CONNECT command
            // Response: 0xFF, resource, comm_mode, max_cto, max_dto, version, protocol
            response[0] = XCP_RES_OK;
            response[1] = 0x01;  // Resource: CAL available
            response[2] = 0x00;  // Comm mode basic
            response[3] = 8;     // Max CTO (Command Transfer Object)
            response[4] = 8;     // Max DTO (Data Transfer Object)
            response[5] = 0x01;  // XCP Version 1.0
            response[6] = 0x01;  // Transport Layer Version
            xcp_connected = 1;
            XCP_SendResponse(response, 7);
            break;
        }

        case XCP_CMD_DISCONNECT: {
            // DISCONNECT command
            response[0] = XCP_RES_OK;
            xcp_connected = 0;
            xcp_mta = 0;
            XCP_SendResponse(response, 1);
            break;
        }

        case XCP_CMD_GET_STATUS: {
            // GET_STATUS command
            response[0] = XCP_RES_OK;
            response[1] = xcp_connected ? 0x01 : 0x00;  // Session status
            response[2] = 0x00;  // Resource protection status
            response[3] = 0x00;  // Reserved
            response[4] = 0x00;  // Session configuration
            XCP_SendResponse(response, 5);
            break;
        }

        case XCP_CMD_SET_MTA: {
            // SET_MTA command
            // Format: CMD, reserved, addr_ext, addr[3:0]
            if (len < 8) {
                XCP_SendError(XCP_ERR_CMD_SYNCH);
                break;
            }

            // Extract 32-bit address (little-endian)
            xcp_mta = ((uint32_t)data[7] << 24) |
                      ((uint32_t)data[6] << 16) |
                      ((uint32_t)data[5] << 8)  |
                      ((uint32_t)data[4]);

            response[0] = XCP_RES_OK;
            XCP_SendResponse(response, 1);
            break;
        }

        case XCP_CMD_UPLOAD: {
            // UPLOAD command
            // Format: CMD, size
            if (len < 2) {
                XCP_SendError(XCP_ERR_CMD_SYNCH);
                break;
            }

            uint8_t size = data[1];
            if (size > (XCP_MAX_PACKET_SIZE - 1)) {
                XCP_SendError(XCP_ERR_OUT_OF_RANGE);
                break;
            }

            // Read from MTA
            response[0] = XCP_RES_OK;
            if (xcp_mta != 0) {
                memcpy(&response[1], (void*)xcp_mta, size);
                xcp_mta += size;
            } else {
                memset(&response[1], 0, size);
            }

            XCP_SendResponse(response, size + 1);
            break;
        }

        case XCP_CMD_SHORT_UPLOAD: {
            // SHORT_UPLOAD command
            // Format: CMD, size, reserved, addr_ext, addr[3:0]
            if (len < 8) {
                XCP_SendError(XCP_ERR_CMD_SYNCH);
                break;
            }

            uint8_t size = data[1];
            if (size > (XCP_MAX_PACKET_SIZE - 1)) {
                XCP_SendError(XCP_ERR_OUT_OF_RANGE);
                break;
            }

            // Extract address
            uint32_t addr = ((uint32_t)data[7] << 24) |
                           ((uint32_t)data[6] << 16) |
                           ((uint32_t)data[5] << 8)  |
                           ((uint32_t)data[4]);

            // Read from address
            response[0] = XCP_RES_OK;
            if (addr != 0) {
                memcpy(&response[1], (void*)addr, size);
            } else {
                memset(&response[1], 0, size);
            }

            XCP_SendResponse(response, size + 1);
            break;
        }

        case XCP_CMD_DOWNLOAD: {
            // DOWNLOAD command
            // Format: CMD, size, data[0...size-1]
            if (len < 2) {
                XCP_SendError(XCP_ERR_CMD_SYNCH);
                break;
            }

            uint8_t size = data[1];
            if (len < (2 + size)) {
                XCP_SendError(XCP_ERR_CMD_SYNCH);
                break;
            }

            // Write to MTA
            if (xcp_mta != 0) {
                memcpy((void*)xcp_mta, &data[2], size);
                xcp_mta += size;
            } else {
                XCP_SendError(XCP_ERR_ACCESS_DENIED);
                break;
            }

            response[0] = XCP_RES_OK;
            XCP_SendResponse(response, 1);
            break;
        }

        case XCP_CMD_SYNCH: {
            // SYNCH command - always responds with error
            XCP_SendError(XCP_ERR_CMD_SYNCH);
            break;
        }

        default:
            XCP_SendError(XCP_ERR_UNKNOWN);
            break;
    }
}

void XCP_UpdateMeasurements(void) {
    // Periodic function to copy variable data to DAQ lists
    // This would be called periodically to update ODT entries
    // For now, variables can be accessed directly via UPLOAD commands
    // using their memory addresses (&hpid_bat.kp, &hkf.x[0], etc.)
}
