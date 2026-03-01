/**
 * @file telemetry.c
 * @brief 텔레메트리 프레임 생성 및 전송
 * @details 148바이트 센서 스냅샷 프레임 (50Hz)
 *          - 헤더: Magic(2), Version(1), Type(1), Seq(4), Timestamp(4), PayloadLen(2)
 *          - 페이로드: 센서 데이터 (134바이트)
 *          - CRC16: CCITT-FALSE (Poly 0x1021, Init 0xFFFF)
 *          - 전송: UART3 (115200 bps)
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include "telemetry.h"
#include "main.h"
#include <stdbool.h>

#ifdef HOST_TEST_MODE
#include <string.h>
#include <stdio.h>
#endif

/**
 * @brief CRC16-CCITT 계산 (CCITT-FALSE)
 * @param data 데이터 버퍼
 * @param length 데이터 길이 (바이트)
 * @return uint16_t CRC16 값
 * @details 다항식: 0x1021, 초기값: 0xFFFF, XOR Out: 0x0000
 */
uint16_t CRC16_CCITT(uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFFU;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000U) {
                crc = (crc << 1) ^ 0x1021U;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

#ifdef HOST_TEST_MODE
static void Telemetry_PrintFrame(telemetry_frame_t *frame) {
    telemetry_payload_sensor_snapshot_t *p = &frame->payload;
    
    printf("\n=== TELEMETRY FRAME ===\n");
    printf("Frame Size: %u bytes\n", (unsigned)sizeof(telemetry_frame_t));
    printf("Header: magic=%02X %02X, ver=%u, type=0x%02X, seq=%u, ts=%ums\n",
           frame->magic[0], frame->magic[1], frame->version, 
           frame->msg_type, frame->seq, frame->timestamp_ms);
    
    printf("--- Sensor Payload ---\n");
    printf("Uptime: %u ms, Status: 0x%04X, CO2: %u ppm\n", 
           p->uptime_ms, p->status_flags, p->co2_ppm);
    
    printf("IMU Accel: X=%.3f Y=%.3f Z=%.3f m/s2\n",
           p->accel_mps2_x1000[0] / 1000.0,
           p->accel_mps2_x1000[1] / 1000.0,
           p->accel_mps2_x1000[2] / 1000.0);
    printf("IMU Gyro:  X=%.3f Y=%.3f Z=%.3f rad/s\n",
           p->gyro_rads_x1000[0] / 1000.0,
           p->gyro_rads_x1000[1] / 1000.0,
           p->gyro_rads_x1000[2] / 1000.0);
    
    printf("Mag: X=%.1f Y=%.1f Z=%.1f uT\n",
           p->mag_uT[0], p->mag_uT[1], p->mag_uT[2]);
    
    printf("Temp: Board=%.2fC, Ext=%.2fC, SHT31=%.2fC, Bat=%.2fC\n",
           p->board_temp_c_x100 / 100.0,
           p->external_temp_c_x100 / 100.0,
           p->sht31_temp_c_x100 / 100.0,
           p->bat_temp_c_x100 / 100.0);
    
    printf("GPS: %.7fN, %.7fE, Alt=%.1fm, Fix=%u, Sats=%u/%u\n",
           p->gps_lat_deg_e7 / 1e7,
           p->gps_lon_deg_e7 / 1e7,
           p->gps_alt_m, p->gps_fix, p->gps_sats_used, p->gps_sats_in_view_total);
    
    printf("Battery: %u mV\n", p->bat_mv);
    
    printf("Air: PM1=%u PM2.5=%u PM10=%u ug/m3, O3=%d ppb\n",
           p->pm1_ugm3, p->pm25_ugm3, p->pm10_ugm3, p->ozone_ppb);
    
    printf("Pressure: %u Pa (%.2fC), Humidity: %.2f%%\n",
           p->ms5611_press_pa, p->ms5611_temp_c_x100 / 100.0, p->sht31_rh_x100 / 100.0);
    
    printf("Radiation: %.2f uSv/h\n", p->gdk101_usvh_x100 / 100.0);
    
    printf("Heater: Bat=%u%% Board=%u%%\n",
           p->heater_bat_duty_percent, p->heater_board_duty_percent);
    
    printf("Altitude: Press=%.1fm, KF=%.1fm\n", p->press_alt_m, p->kf_alt_m);
    printf("Attitude: Roll=%.2f deg, Pitch=%.2f deg\n", p->kf_roll_deg, p->kf_pitch_deg);
    
    printf("CRC16: 0x%04X\n", frame->crc16);
    printf("===========================\n");
}
#endif

extern UART_HandleTypeDef huart3;
static volatile bool tx_busy = false;

void Telemetry_Send(telemetry_frame_t *frame) {
    if (tx_busy) {
        // Previous transmission still in progress.
        // Skip this frame to preserve real-time loop.
        return;
    }

    /* Frame size = sizeof(telemetry_frame_t).
     * CRC applies to bytes 0 to end-3 (Total - 2 bytes for CRC)
     */
    uint16_t total_len = sizeof(telemetry_frame_t);
    uint16_t crc_len = total_len - 2U;
    
    frame->crc16 = CRC16_CCITT((uint8_t*)frame, crc_len);
    
#ifdef HOST_TEST_MODE
    Telemetry_PrintFrame(frame);
    // Simulate DMA completion for test
    tx_busy = false; 
#else
    /* Non-blocking DMA Transmission */
    // MISRA C 2023: Protect tx_busy race condition
    uint32_t primask_bit = __get_PRIMASK();
    __disable_irq();
    tx_busy = true;
    __set_PRIMASK(primask_bit);
    
    if (HAL_UART_Transmit_DMA(&huart3, (uint8_t*)frame, total_len) != HAL_OK) {
        // Error handling: Reset busy flag if start failed
        tx_busy = false;
    }
#endif
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) {
        tx_busy = false;
    }
}
