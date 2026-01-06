#include "app.h"
#include "fdir.h"
#include "main.h"  // For HAL_GPIO and pin definitions
#include "sensors.h" // For SensorID definitions
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// Global Handles
PID_HandleTypeDef hpid_bat;
PID_HandleTypeDef hpid_brd;
KF_Handle_t hkf;

// Telemetry Data
telemetry_frame_t telem_frame;

// Control Outputs
float heater_battery_cmd = 0.0f;
float heater_board_cmd = 0.0f;

void App_Init(void) {
    // 1. Sensor Init
    Sensors_Init();

    // 2. Thermal PID Init
    // Heater max output 100.0 (percent)
    PID_Init(&hpid_bat, 1000.0f, 10.0f, 0.0f, 100.0f); // Kp, Ki, Kd, Max
    hpid_bat.Target = 10.0f; // Maintain 10C
    
    PID_Init(&hpid_brd, 500.0f, 5.0f, 0.0f, 100.0f);
    hpid_brd.Target = 5.0f; // Maintain 5C
    
    // 3. Kalman Init (50Hz = 0.02s)
    // Process Noise 0.5 (Balloon Dynamics), Meas Noise 0.3 (Baro Precision ~0.5m)
    KF_Init(&hkf, 0.02f, 0.5f, 0.3f);
    
    // 4. XCP Init
    XCP_Init();
    
    // 5. Telemetry Header Init
    telem_frame.magic[0] = 0xA5;
    telem_frame.magic[1] = 0x5A;
    telem_frame.version = 1;
    telem_frame.msg_type = 0x02; // Sensor Snapshot
    telem_frame.payload_len = sizeof(telemetry_payload_sensor_snapshot_t);
    
    // 6. Actuators Init
    Actuators_Init();
    
    // 7. FDIR Init
    FDIR_Init();
}

void App_Loop(void) {
    // 1. Get Sensor Data
    Sensors_Read_All(&telem_frame.payload);
    
    // 4. Air Quality (CO2, Ozone, PM)
    Sensors_Read_AirQuality(&telem_frame.payload.co2_ppm, &telem_frame.payload.ozone_ppb, &telem_frame.payload.pm1_ugm3, &telem_frame.payload.pm25_ugm3);
    
    // Convert fixed point to float for Algorithms
    // 5. Battery
    Sensors_Read_Battery(&telem_frame.payload.bat_mv, &telem_frame.payload.bat_temp_c_x100);
    
    // 6. Others
    Sensors_Read_BoardTemp(&telem_frame.payload.board_temp_c_x100);
    Sensors_Read_External(&telem_frame.payload.external_temp_c_x100);
    Sensors_Read_Rad(&telem_frame.payload.gdk101_usvh_x100);
    
    // ** Temperature-based FDIR Update **
    // Update FDIR with current external temperature for thermal protection logic
    int16_t ext_temp = telem_frame.payload.external_temp_c_x100;
    FDIR_UpdateTemperature(ext_temp);
    
    // Check FDIR status for Air Quality Sensors and mask data if disabled/failed
    if (!FDIR_IsSensorHealthy(SENSOR_ID_PMS) || FDIR_IsSensorColdDisabled(SENSOR_ID_PMS)) {
        telem_frame.payload.pm1_ugm3 = 0xFFFF;
        telem_frame.payload.pm25_ugm3 = 0xFFFF;
        telem_frame.payload.pm10_ugm3 = 0xFFFF;
    }
    
    if (!FDIR_IsSensorHealthy(SENSOR_ID_CO2) || FDIR_IsSensorColdDisabled(SENSOR_ID_CO2)) {
        telem_frame.payload.co2_ppm = 0xFFFF;
    }
    
    if (!FDIR_IsSensorHealthy(SENSOR_ID_RAD) || FDIR_IsSensorColdDisabled(SENSOR_ID_RAD)) {
        telem_frame.payload.gdk101_usvh_x100 = 0xFFFF;
    }
     
    // Ozone sensor special handling (ppb)
    if (!FDIR_IsSensorHealthy(SENSOR_ID_SHT) && !FDIR_IsSensorHealthy(SENSOR_ID_EXT_TEMP)) {
       // If both temp sensors fail, we might want to flag something, but currently just proceed
    }

    // ** SHT31 Heater Control (Anti-condensation) **
    // Turn ON if temp < 0C, Turn OFF if temp > 2C (Hysteresis)
    float sht31_temp_c = telem_frame.payload.sht31_temp_c_x100 / 100.0f;
    static uint8_t sht31_heater_on = 0;

    if (sht31_temp_c < 0.0f) {
        if (sht31_heater_on == 0) {
            Sensors_SetHeater_SHT31(1);
            sht31_heater_on = 1;
        }
    } else if (sht31_temp_c > 2.0f) {
        if (sht31_heater_on == 1) {
            Sensors_SetHeater_SHT31(0);
            sht31_heater_on = 0;
        }
    }

    
    // 7. GPS
    Sensors_Read_GPS(&telem_frame.payload.gps_lat_deg_e7, &telem_frame.payload.gps_lon_deg_e7, 
                     &telem_frame.payload.gps_alt_m, &telem_frame.payload.gps_fix,
                     &telem_frame.payload.gps_sats_used, &telem_frame.payload.gps_sats_in_view_total,
                     &telem_frame.payload.gps_sats_in_view_gps, &telem_frame.payload.gps_sats_in_view_glonass,
                     &telem_frame.payload.gps_sats_in_view_galileo, &telem_frame.payload.gps_sats_in_view_beidou,
                     &telem_frame.payload.gps_utc_hour, &telem_frame.payload.gps_utc_min, &telem_frame.payload.gps_utc_sec,
                     &telem_frame.payload.gps_utc_day, &telem_frame.payload.gps_utc_month, &telem_frame.payload.gps_utc_year);
    
    // ** FDIR GPS Altitude Tracking (Range + Continuity) **
    FDIR_UpdateGPSAltitude(telem_frame.payload.gps_alt_m);
    
    // ** Attitude Estimation (SFLP) **
    float quat[4]; // x, y, z, w
    float roll_deg = 0.0f;
    float pitch_deg = 0.0f;
    
    Sensors_Read_SFLP(quat);
    
    // Quaternion to Euler (Roll, Pitch) conversion
    // Assuming quat order: [x, y, z, w]
    float qx = quat[0];
    float qy = quat[1];
    float qz = quat[2];
    float qw = quat[3];
    
    // Roll (x-axis rotation)
    float sinr_cosp = 2.0f * (qw * qx + qy * qz);
    float cosr_cosp = 1.0f - 2.0f * (qx * qx + qy * qy);
    roll_deg = atan2f(sinr_cosp, cosr_cosp) * (180.0f / 3.14159265f);
    
    // Pitch (y-axis rotation)
    float sinp = 2.0f * (qw * qy - qz * qx);
    if (fabsf(sinp) >= 1)
        pitch_deg = copysignf(90.0f, sinp); // use 90 degrees if out of range
    else
        pitch_deg = asinf(sinp) * (180.0f / 3.14159265f);
        
    telem_frame.payload.kf_roll_deg = roll_deg;
    telem_frame.payload.kf_pitch_deg = pitch_deg;
    
    // Convert fixed point to float for Algorithms
    float current_battery_temp = telem_frame.payload.bat_temp_c_x100 / 100.0f;
    float current_board_temp = telem_frame.payload.board_temp_c_x100 / 100.0f;
    
    /* FDIR Baro Range Validation */
    if (!FDIR_ValidateRange_Baro(telem_frame.payload.ms5611_press_pa)) {
        telem_frame.payload.ms5611_press_pa = 101325U; /* Use sea level as fallback */
    }
    
    // Barometric Altitude (Approx)
    // P0=101325, Lapse Rate can be added later. Linear approx near sea level: 12Pa per meter.
    if (telem_frame.payload.ms5611_press_pa == 0) telem_frame.payload.ms5611_press_pa = 101325; // Prevent jump if 0
    float baro_alt = (101325.0f - (float)telem_frame.payload.ms5611_press_pa) / 12.0f;
    
    // ** FDIR Baro Altitude Tracking **
    FDIR_UpdateBaroAltitude(baro_alt); 
    
    telem_frame.payload.press_alt_m = baro_alt; 
    
    /* Kalman Initial Convergence */
    static uint8_t kf_initialized = 0U;
    if ((kf_initialized == 0U) && (baro_alt > -1000.0f) && (baro_alt < 40000.0f)) { 
        hkf.x[0] = baro_alt; /* Initialize State to Measurement */
        kf_initialized = 1U;
    }
    
    // 2. PID Update
    heater_battery_cmd = PID_Update(&hpid_bat, current_battery_temp, 0.02f);
    heater_board_cmd = PID_Update(&hpid_brd, current_board_temp, 0.02f);
    
    // 3. Actuator Output
    Actuators_SetHeater_Battery(heater_battery_cmd);
    Actuators_SetHeater_Board(heater_board_cmd);
    
    // Update Telemetry with Control Output
    telem_frame.payload.heater_bat_duty_percent = (uint8_t)heater_battery_cmd;
    telem_frame.payload.heater_board_duty_percent = (uint8_t)heater_board_cmd;
    
    /* 3. Kalman Update (Predict --> Update) */
    KF_Predict(&hkf);
    KF_Update_Altitude(&hkf, baro_alt);
    
    KF_CheckDivergence(&hkf);  /* FMEA W-05: Check and reset if diverged */
    
    telem_frame.payload.kf_alt_m = hkf.x[0];
    
    // ** FDIR Status Flags Update **
    telem_frame.payload.status_flags = FDIR_GetStatusFlags();
    
    /* Add heater active flag */
    if ((heater_battery_cmd > 1.0f) || (heater_board_cmd > 1.0f)) {
        telem_frame.payload.status_flags |= STATUS_HEATER_ACTIVE;
    }
    
    // 4. Update Header
    telem_frame.seq++;
    telem_frame.timestamp_ms += 20; // Simulated time
    
    // 5. XCP DAQ
    XCP_UpdateMeasurements();
    
    // 6. Telemetry Transmit
    Telemetry_Send(&telem_frame);
    
    // 7. FDIR Update
    FDIR_Update();
}
