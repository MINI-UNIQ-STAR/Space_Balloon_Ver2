#include "app.h"
#include "fdir.h"
#include "bsp.h" // [NEW] BSP Layer
#include <stdio.h> // For printf if needed

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
    // 0. Board Init
    BSP_Init();

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
    
    // 7. GPS
    Sensors_Read_GPS(&telem_frame.payload.gps_lat_deg_e7, &telem_frame.payload.gps_lon_deg_e7, 
                     &telem_frame.payload.gps_alt_m, &telem_frame.payload.gps_fix,
                     &telem_frame.payload.gps_sats_used, &telem_frame.payload.gps_sats_in_view_total,
                     &telem_frame.payload.gps_sats_in_view_gps, &telem_frame.payload.gps_sats_in_view_glonass,
                     &telem_frame.payload.gps_sats_in_view_galileo, &telem_frame.payload.gps_sats_in_view_beidou);
    
    // Convert fixed point to float for Algorithms
    float current_battery_temp = telem_frame.payload.bat_temp_c_x100 / 100.0f;
    float current_board_temp = telem_frame.payload.board_temp_c_x100 / 100.0f;
    
    // Barometric Altitude (Approx)
    // P0=101325, Lapse Rate can be added later. Linear approx near sea level: 12Pa per meter.
    if (telem_frame.payload.ms5611_press_pa == 0) telem_frame.payload.ms5611_press_pa = 101325; // Prevent jump if 0
    float baro_alt = (101325.0f - (float)telem_frame.payload.ms5611_press_pa) / 12.0f; 
    
    telem_frame.payload.press_alt_m = baro_alt; 
    
    // ** Kalman Initial Convergence **
    static uint8_t kf_initialized = 0;
    if (!kf_initialized && baro_alt > -1000.0f && baro_alt < 40000.0f) { // Range check
        hkf.x[0] = baro_alt; // Initialize State to Measurement
        kf_initialized = 1;
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
    
    // 3. Kalman Update
    KF_Update_Altitude(&hkf, baro_alt);
    
    telem_frame.payload.kf_alt_m = hkf.x[0];
    
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
