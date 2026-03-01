#include <stdio.h>
#include <stdint.h>
#include "../Inc/telemetry.h"
#include "../docs/HITL/common/hitl_protocol.h"

int main(void) {
    printf("=== Structure Size Compatibility Check ===\n\n");

    printf("Telemetry Structures:\n");
    printf("  telemetry_payload_sensor_snapshot_t: %zu bytes\n", sizeof(telemetry_payload_sensor_snapshot_t));
    printf("  telemetry_frame_t: %zu bytes\n", sizeof(telemetry_frame_t));

    printf("\nHITL Structures:\n");
    printf("  HitlStatePacket: %zu bytes\n", sizeof(HitlStatePacket));
    printf("  HitlFeedbackPacket: %zu bytes\n", sizeof(HitlFeedbackPacket));

    printf("\n=== Field Offsets in telemetry_payload_sensor_snapshot_t ===\n");
    telemetry_payload_sensor_snapshot_t dummy;
    printf("  uptime_ms: offset=%zu\n", (size_t)&dummy.uptime_ms - (size_t)&dummy);
    printf("  status_flags: offset=%zu\n", (size_t)&dummy.status_flags - (size_t)&dummy);
    printf("  co2_ppm: offset=%zu\n", (size_t)&dummy.co2_ppm - (size_t)&dummy);
    printf("  accel_mps2_x1000: offset=%zu\n", (size_t)&dummy.accel_mps2_x1000 - (size_t)&dummy);
    printf("  gyro_rads_x1000: offset=%zu\n", (size_t)&dummy.gyro_rads_x1000 - (size_t)&dummy);
    printf("  mag_uT: offset=%zu\n", (size_t)&dummy.mag_uT - (size_t)&dummy);
    printf("  board_temp_c_x100: offset=%zu\n", (size_t)&dummy.board_temp_c_x100 - (size_t)&dummy);
    printf("  external_temp_c_x100: offset=%zu\n", (size_t)&dummy.external_temp_c_x100 - (size_t)&dummy);
    printf("  sht31_temp_c_x100: offset=%zu\n", (size_t)&dummy.sht31_temp_c_x100 - (size_t)&dummy);
    printf("  bat_temp_c_x100: offset=%zu\n", (size_t)&dummy.bat_temp_c_x100 - (size_t)&dummy);
    printf("  gps_lat_deg_e7: offset=%zu\n", (size_t)&dummy.gps_lat_deg_e7 - (size_t)&dummy);
    printf("  gps_lon_deg_e7: offset=%zu\n", (size_t)&dummy.gps_lon_deg_e7 - (size_t)&dummy);
    printf("  gps_alt_m: offset=%zu\n", (size_t)&dummy.gps_alt_m - (size_t)&dummy);
    printf("  gps_fix: offset=%zu\n", (size_t)&dummy.gps_fix - (size_t)&dummy);
    printf("  gps_sats_used: offset=%zu\n", (size_t)&dummy.gps_sats_used - (size_t)&dummy);
    printf("  gps_sats_in_view_total: offset=%zu\n", (size_t)&dummy.gps_sats_in_view_total - (size_t)&dummy);
    printf("  gps_sats_in_view_gps: offset=%zu\n", (size_t)&dummy.gps_sats_in_view_gps - (size_t)&dummy);
    printf("  gps_sats_in_view_glonass: offset=%zu\n", (size_t)&dummy.gps_sats_in_view_glonass - (size_t)&dummy);
    printf("  gps_sats_in_view_galileo: offset=%zu\n", (size_t)&dummy.gps_sats_in_view_galileo - (size_t)&dummy);
    printf("  gps_sats_in_view_beidou: offset=%zu\n", (size_t)&dummy.gps_sats_in_view_beidou - (size_t)&dummy);
    printf("  gps_utc_hour: offset=%zu\n", (size_t)&dummy.gps_utc_hour - (size_t)&dummy);
    printf("  gps_utc_min: offset=%zu\n", (size_t)&dummy.gps_utc_min - (size_t)&dummy);
    printf("  gps_utc_sec: offset=%zu\n", (size_t)&dummy.gps_utc_sec - (size_t)&dummy);
    printf("  gps_utc_day: offset=%zu\n", (size_t)&dummy.gps_utc_day - (size_t)&dummy);
    printf("  gps_utc_month: offset=%zu\n", (size_t)&dummy.gps_utc_month - (size_t)&dummy);
    printf("  gps_utc_year: offset=%zu\n", (size_t)&dummy.gps_utc_year - (size_t)&dummy);
    printf("  bat_mv: offset=%zu\n", (size_t)&dummy.bat_mv - (size_t)&dummy);
    printf("  pm1_ugm3: offset=%zu\n", (size_t)&dummy.pm1_ugm3 - (size_t)&dummy);
    printf("  kf_alt_m: offset=%zu\n", (size_t)&dummy.kf_alt_m - (size_t)&dummy);
    printf("  kf_roll_deg: offset=%zu\n", (size_t)&dummy.kf_roll_deg - (size_t)&dummy);
    printf("  kf_pitch_deg: offset=%zu\n", (size_t)&dummy.kf_pitch_deg - (size_t)&dummy);

    printf("\n=== Compatibility Check Results ===\n");
    if (sizeof(telemetry_payload_sensor_snapshot_t) == 134) {
        printf("✓ Telemetry payload size is correct (134 bytes with UTC fields)\n");
    } else {
        printf("✗ WARNING: Telemetry payload size mismatch! Expected 134, got %zu\n",
               sizeof(telemetry_payload_sensor_snapshot_t));
    }

    return 0;
}
