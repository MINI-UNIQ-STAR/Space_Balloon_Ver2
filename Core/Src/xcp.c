#include "xcp.h"
#include "bsp.h" // For BSP_UART_Write
#include <string.h>

// Global Algorithm Instances (External linkage)
extern PID_HandleTypeDef hpid_bat;
extern PID_HandleTypeDef hpid_brd;
extern KF_Handle_t hkf;

// Command/Response Buffers
static uint8_t xcp_rx_buf[XCP_MAX_PACKET_SIZE];
static uint8_t xcp_tx_buf[XCP_MAX_PACKET_SIZE];

void XCP_Init(void) {
    // Initialize XCP Protocol Layer
    // In a real implementation: Setup CAN/UART interrupts, Timer for DAQ
    memset(xcp_rx_buf, 0, XCP_MAX_PACKET_SIZE);
    memset(xcp_tx_buf, 0, XCP_MAX_PACKET_SIZE);
}

// XCP Commands
#define CC_CONNECT          0xFF
#define CC_SHORT_UPLOAD     0xF4
#define CC_SHORT_DOWNLOAD   0xF0
#define CC_GET_STATUS       0xFD

// XCP Responses
#define PID_RES             0xFF
#define ERR_CMD_UNKNOWN     0x20
#define ERR_ACCESS_DENIED   0x24

// State
typedef enum {
    XCP_DISCONNECTED = 0,
    XCP_CONNECTED = 1
} XCP_State_t;

static XCP_State_t xcp_state = XCP_DISCONNECTED;

// Helper to send response
static void XCP_SendResponse(uint8_t len) {
    BSP_UART_Write(xcp_tx_buf, len);
}

void XCP_ProcessCommand(uint8_t *data, uint8_t len) {
    if (len == 0) return;
    
    uint8_t cmd = data[0];
    
    // CONNECT
    if (cmd == CC_CONNECT) {
        xcp_state = XCP_CONNECTED;
        
        xcp_tx_buf[0] = PID_RES;
        xcp_tx_buf[1] = 0x00; // Resource (Cal/Pag)
        xcp_tx_buf[2] = 0x00; // Comm Mode
        xcp_tx_buf[3] = XCP_MAX_PACKET_SIZE; // Max CTO
        xcp_tx_buf[4] = XCP_MAX_PACKET_SIZE; // Max DTO (low byte)
        xcp_tx_buf[5] = 0x00; // Max DTO (high byte)
        xcp_tx_buf[6] = 0x01; // Proto Ver
        xcp_tx_buf[7] = 0x01; // Transport Ver
        
        XCP_SendResponse(8);
        return;
    }
    
    if (xcp_state != XCP_CONNECTED) {
        // Ignore other commands if not connected? 
        // Or send error? Standard says silent or error.
        return; 
    }
    
    // SHORT_UPLOAD (Read)
    // Packet: [Cmd][Len][Res][Addr32]
    if (cmd == CC_SHORT_UPLOAD) {
        if (len < 8) return; // Malformed
        
        uint8_t num_bytes = data[1];
        // Address is at offset 4 (Little Endian in internal logic but standard is usually LE)
        // Let's assume Addr is at data[4]..data[7]
        uint32_t addr = (uint32_t)data[4] | ((uint32_t)data[5] << 8) | ((uint32_t)data[6] << 16) | ((uint32_t)data[7] << 24);
        
        if (num_bytes > (XCP_MAX_PACKET_SIZE - 1)) {
            xcp_tx_buf[0] = 0xFE; // ERR_OUT_OF_RANGE (Simplification)
            XCP_SendResponse(2);
            return;
        }

        // Safety: Prevent reading NULL or restricted?
        // In SIL, we trust the tool.
        uint8_t *ptr = (uint8_t*)(uintptr_t)addr; // Cast 32-bit addr to pointer
        
        xcp_tx_buf[0] = PID_RES;
        if (ptr != NULL) {
             memcpy(&xcp_tx_buf[1], ptr, num_bytes);
        } else {
             memset(&xcp_tx_buf[1], 0, num_bytes);
        }
        
        XCP_SendResponse(1 + num_bytes);
        return;
    }
    
    // SHORT_DOWNLOAD (Write)
    // Packet: [Cmd][Len][Res][Addr32][Data...]
    if (cmd == CC_SHORT_DOWNLOAD) {
        if (len < 8) return;
        
        uint8_t num_bytes = data[1];
        uint32_t addr = (uint32_t)data[4] | ((uint32_t)data[5] << 8) | ((uint32_t)data[6] << 16) | ((uint32_t)data[7] << 24);
        
        if (len < (8 + num_bytes)) return; // Not enough data
        
        uint8_t *ptr = (uint8_t*)(uintptr_t)addr;
        
        if (ptr != NULL) {
            memcpy(ptr, &data[8], num_bytes);
        }
        
        xcp_tx_buf[0] = PID_RES;
        XCP_SendResponse(1);
        return;
    }
    
    // Unknown Command
    xcp_tx_buf[0] = 0xFE; // ERR_CMD_UNKNOWN
    xcp_tx_buf[1] = ERR_CMD_UNKNOWN;
    XCP_SendResponse(2);
}

void XCP_UpdateMeasurements(void) {
    // Periodic function to copy variable data to DAQ lists
    // Access hpid_bat.kp, hkf.x[0] etc.
}
