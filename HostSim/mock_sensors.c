#include "sensors.h"
#include "flight_data.h"
#include "fdir.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Flight data playback state
static uint32_t current_frame = 0;
static uint32_t loop_count = 0;

// ===== Fault Injection for FDIR Testing =====
typedef enum {
    FAULT_NONE = 0,
    FAULT_BARO_RANGE,      // Inject out-of-range pressure
    FAULT_GPS_JUMP,        // Inject altitude jump
    FAULT_BARO_TIMEOUT,    // Stop reporting baro data
    FAULT_GPS_TIMEOUT,     // Stop reporting GPS data
} FaultType_t;

static FaultType_t active_fault = FAULT_NONE;
static uint32_t fault_start_frame = 0;
static uint32_t fault_duration_frames = 50; // ~1 second at 50Hz

void MockSensors_InjectFault(FaultType_t fault, uint32_t at_frame, uint32_t duration) {
    active_fault = fault;
    fault_start_frame = at_frame;
    fault_duration_frames = duration;
    printf("[MockFault] Injecting fault %d at frame %u for %u frames\n", fault, at_frame, duration);
}

void MockSensors_ClearFault(void) {
    active_fault = FAULT_NONE;
    printf("[MockFault] Fault cleared\n");
}

static bool _is_fault_active(void) {
    return (active_fault != FAULT_NONE && 
            current_frame >= fault_start_frame && 
            current_frame < fault_start_frame + fault_duration_frames);
}

// Interpolation helper
static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

void Sensors_Init(void) {
    printf("[Mock] Sensors Initialized - RS41 Flight Data Mode\n");
    printf("[Mock] Loaded %d flight data points\n", FLIGHT_DATA_COUNT);
    printf("[Mock] Altitude range: %.0fm - %.0fm\n", 
           flight_data[0].alt_m, 
           flight_data[FLIGHT_DATA_COUNT-1].alt_m);
    current_frame = 0;
    loop_count = 0;
    active_fault = FAULT_NONE;
}

void Sensors_Reset(SensorID_t id) {
    printf("[Mock] Sensor Reset: %d\n", id); 
}

// Stub other inits
void Sensors_Init_I2C1(void) {}
void Sensors_Init_I2C3(void) {}
void Sensors_Init_UART(void) {}
void Sensors_Init_1Wire(void) {}

SensorStatus_t Sensors_Read_All(telemetry_payload_sensor_snapshot_t *data) {
    // Get current and next frame for interpolation
    const flight_data_point_t *cur = &flight_data[current_frame];
    const flight_data_point_t *next = &flight_data[(current_frame + 1) % FLIGHT_DATA_COUNT];
    
    // Interpolation factor (10 sub-steps per frame for smoother playback)
    float t = (loop_count % 10) / 10.0f;
    
    // GPS Data (interpolated)
    data->gps_lat_deg_e7 = (int32_t)lerp((float)cur->lat_e7, (float)next->lat_e7, t);
    data->gps_lon_deg_e7 = (int32_t)lerp((float)cur->lon_e7, (float)next->lon_e7, t);
    data->gps_alt_m = lerp(cur->alt_m, next->alt_m, t);
    data->gps_fix = 1;
    data->gps_sats_used = cur->sats;
    data->gps_sats_in_view_total = cur->sats + 3;
    data->gps_sats_in_view_gps = cur->sats;
    data->gps_sats_in_view_glonass = 2;
    data->gps_sats_in_view_galileo = 1;
    data->gps_sats_in_view_beidou = 0;
    
    // Temperature (from radiosonde data or altitude model)
    float temp_c = cur->temp_c;
    if (temp_c < -100.0f) {
        // If temp not available, use ISA model: -6.5°C per 1000m
        temp_c = 15.0f - (data->gps_alt_m * 0.0065f);
    }
    data->external_temp_c_x100 = (int16_t)(temp_c * 100);
    data->sht31_temp_c_x100 = (int16_t)((temp_c + 30.0f) * 100); // Board temp warmer
    data->board_temp_c_x100 = (int16_t)((temp_c + 35.0f) * 100);
    data->bat_temp_c_x100 = (int16_t)((temp_c + 40.0f) * 100);   // Battery temp even warmer
    
    // Pressure from altitude (ISA barometric formula)
    float pressure_pa = 101325.0f * powf(1.0f - data->gps_alt_m / 44330.0f, 5.255f);
    data->ms5611_press_pa = (uint32_t)pressure_pa;
    data->ms5611_temp_c_x100 = data->external_temp_c_x100;
    
    // Battery (from radiosonde data)
    data->bat_mv = cur->batt_mv;
    
    // IMU - simulate based on velocity changes
    float accel_z = 9810.0f; // 1G in x1000 scale
    if (cur->vel_v > 0) {
        accel_z += (int32_t)(cur->vel_v * 100); // Slight acceleration during ascent
    }
    data->accel_mps2_x1000[0] = 0;
    data->accel_mps2_x1000[1] = 0;
    data->accel_mps2_x1000[2] = (int32_t)accel_z;
    
    // Gyro - small random oscillations
    data->gyro_rads_x1000[0] = (loop_count % 3) - 1;
    data->gyro_rads_x1000[1] = (loop_count % 5) - 2;
    data->gyro_rads_x1000[2] = (loop_count % 7) - 3;
    
    // Magnetometer - heading based
    float heading_rad = cur->heading_deg * 3.14159f / 180.0f;
    data->mag_uT[0] = 25.0f * cosf(heading_rad);
    data->mag_uT[1] = 25.0f * sinf(heading_rad);
    data->mag_uT[2] = 45.0f;
    
    // Humidity - decreases with altitude
    float rh = 50.0f - (data->gps_alt_m / 200.0f);
    if (rh < 5.0f) rh = 5.0f;
    data->sht31_rh_x100 = (uint16_t)(rh * 100);
    
    // Air quality - mock values
    data->co2_ppm = 400;
    data->pm1_ugm3 = 10;
    data->pm25_ugm3 = 15;
    data->pm10_ugm3 = 20;
    data->ozone_ppb = 30;
    
    // Radiation - increases with altitude
    float rad = 0.1f + (data->gps_alt_m / 5000.0f) * 0.5f;
    data->gdk101_usvh_x100 = (uint16_t)(rad * 100);
    
    // Advance frame counter
    loop_count++;
    if (loop_count % 10 == 0) {
        current_frame++;
        if (current_frame >= FLIGHT_DATA_COUNT) {
            current_frame = 0;
            printf("[Mock] Flight data looped\n");
        }
    }
    
    // Report success to FDIR for all sensors (since this is mock healthy data)
    FDIR_ReportSuccess(SENSOR_ID_GPS);
    FDIR_ReportSuccess(SENSOR_ID_BARO);
    FDIR_ReportSuccess(SENSOR_ID_IMU);
    FDIR_ReportSuccess(SENSOR_ID_MAG);
    FDIR_ReportSuccess(SENSOR_ID_SHT);
    FDIR_ReportSuccess(SENSOR_ID_EXT_TEMP);
    FDIR_ReportSuccess(SENSOR_ID_PMS);
    FDIR_ReportSuccess(SENSOR_ID_CO2);
    FDIR_ReportSuccess(SENSOR_ID_RAD);

    return SENSOR_OK;
}

// Individual mocks if App calls them
void Sensors_Read_IMU(int32_t accel[3], int32_t gyro[3]) {
    accel[0] = 0;
    accel[1] = 0;
    accel[2] = 9810;
    gyro[0] = 0;
    gyro[1] = 0;
    gyro[2] = 0;
}

void Sensors_Read_SFLP(float quaternion[4]) {
    // Simulate slow rotation/tumbling during ascent
    // Simple rotation around X axis (Roll)
    static float angle = 0.0f;
    angle += 0.01f; // Increment angle
    
    // Euler to Quaternion (Roll only)
    // qx = sin(roll/2) * cos(pitch/2) * cos(yaw/2) - cos(roll/2) * sin(pitch/2) * sin(yaw/2)
    // Here pitch=0, yaw=0
    // qx = sin(angle/2)
    // qw = cos(angle/2)
    
    quaternion[0] = sinf(angle * 0.5f); // x
    quaternion[1] = 0.0f;               // y
    quaternion[2] = 0.0f;               // z
    quaternion[3] = cosf(angle * 0.5f); // w (scalar)
}

void Sensors_Read_Mag(float mag[3]) {
    mag[0] = 25.0f;
    mag[1] = 0.0f;
    mag[2] = 45.0f;
}

void Sensors_Read_Rad(uint16_t *uSvh) {
    *uSvh = 15; // 0.15 uSv/h
}

void Sensors_Read_Baro(uint32_t *press_pa, int16_t *temp_c_x100) {
    *press_pa = 50000; // ~5500m altitude
    *temp_c_x100 = -2800; // -28°C
}

void Sensors_Read_Humid(int16_t *temp_c_x100, uint16_t *rh_x100) {
    *temp_c_x100 = -2500;
    *rh_x100 = 2000;
}

void Sensors_Read_AirQuality(uint16_t *co2, int16_t *ozone, uint16_t *pm1_0, uint16_t *pm2_5) {
    *co2 = 400;
    *ozone = 30;
    *pm1_0 = 10;
    *pm2_5 = 15;
}

void Sensors_Read_Battery(uint16_t *mv, int16_t *temp_c_x100) {
    *mv = flight_data[current_frame].batt_mv;
    *temp_c_x100 = 1000; // 10°C battery
}

void Sensors_Read_BoardTemp(int16_t *temp_c_x100) {
    *temp_c_x100 = 500; // 5°C
}

void Sensors_Read_External(int16_t *temp_c_x100) {
    *temp_c_x100 = (int16_t)(flight_data[current_frame].temp_c * 100);
}

void Sensors_Read_GPS(int32_t *lat, int32_t *lon, float *alt, uint8_t *fix, 
                      uint8_t *sats, uint8_t *sats_view,
                      uint8_t *sats_gps, uint8_t *sats_glonass,
                      uint8_t *sats_galileo, uint8_t *sats_beidou,
                      uint8_t *utc_hour, uint8_t *utc_min, uint8_t *utc_sec,
                      uint8_t *utc_day, uint8_t *utc_month, uint16_t *utc_year) {
    const flight_data_point_t *cur = &flight_data[current_frame];
    *lat = cur->lat_e7;
    *lon = cur->lon_e7;
    *alt = cur->alt_m;
    *fix = 1;
    *sats = cur->sats;
    *sats_view = cur->sats + 3;
    *sats_gps = cur->sats;
    *sats_glonass = 2;
    *sats_galileo = 1;
    *sats_beidou = 0;
    
    // Mock UTC Time
    *utc_hour = 12; *utc_min = 0; *utc_sec = 0;
    *utc_day = 1; *utc_month = 1; *utc_year = 2026;
}
