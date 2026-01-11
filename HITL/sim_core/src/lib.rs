use pyo3::prelude::*;
use rand::Rng;
use std::f64::consts::PI;

#[pyclass]
struct RustSimulator {
    // State
    lat: f64,
    lon: f64,
    alt: f64,
    start_time: f64,
    
    // Sensor Values
    press_pa: i32,
    temp_c: i32,
    acc_x: i32,
    acc_y: i32,
    acc_z: i32,
    acc_z_g: f64, // Physical value for GUI
    rad_x100: i32,
    co2: i32,
    
    // Fault Injection Flags
    gps_timeout: bool,
    gps_fix_loss: bool,
    baro_freeze: bool,
    imu_freeze: bool,
    fault_end_time: std::collections::HashMap<String, f64>,
}

#[pymethods]
impl RustSimulator {
    #[new]
    fn new(lat: f64, lon: f64, alt: f64) -> Self {
        RustSimulator {
            lat, lon, alt,
            start_time: 0.0, // Set by init or first update
            press_pa: 101325,
            temp_c: 2500,
            acc_x: 0, acc_y: 0, acc_z: 9800,
            acc_z_g: 9.8,
            rad_x100: 10,
            co2: 400,
            gps_timeout: false,
            gps_fix_loss: false,
            baro_freeze: false,
            imu_freeze: false,
            fault_end_time: std::collections::HashMap::new(),
        }
    }

    fn update(&mut self, current_time: f64) {
        if self.start_time == 0.0 {
            self.start_time = current_time;
        }
        let elapsed = current_time - self.start_time;

        // 1. Env Simulation
        // Pressure Formula: P = 101325 * (1 - 2.25577e-5 * h)^5.25588
        let press_hpa = 1013.25 * (1.0 - 2.25577e-5 * self.alt).powf(5.25588);
        
        if !self.baro_freeze {
            self.press_pa = (press_hpa * 100.0) as i32;
        }

        let temp = 25.0 - (0.0065 * self.alt);
        self.temp_c = (temp * 100.0) as i32;

        // 2. IMU Simulation
        let mut rng = rand::thread_rng();
        
        let noise_x = rng.gen_range(-0.1..0.1);
        let noise_y = rng.gen_range(-0.1..0.1);
        let noise_z = rng.gen_range(-0.5..0.5);
        
        let raw_acc_z = 9.8 + noise_z;
        self.acc_z_g = raw_acc_z; // Store float for GUI

        if !self.imu_freeze {
            self.acc_x = (noise_x * 1000.0) as i32;
            self.acc_y = (noise_y * 1000.0) as i32;
            self.acc_z = (raw_acc_z * 1000.0) as i32;
        }

        // 3. Payload
        self.co2 = 400 + (self.alt / 10.0) as i32 + rng.gen_range(-10..10);
        let rad = 0.1 + (self.alt / 5000.0) * 2.0;
        self.rad_x100 = (rad * 100.0) as i32;
        
        // 4. Fault Management (Simple Timer Check)
        // In a real implementation, we would check self.fault_end_time against current_time
        // and clear flags if expired.
    }

    fn inject_fault(&mut self, component: String, fault_type: String, duration: f64, now: f64) {
        // Simple flag setting for demo
        let end_t = now + duration;
        match component.as_str() {
            "GPS" => if fault_type == "TIMEOUT" { self.gps_timeout = true; },
            "BARO" => if fault_type == "FREEZE" { self.baro_freeze = true; },
            "IMU" => if fault_type == "FREEZE" { self.imu_freeze = true; },
            _ => {},
        }
        self.fault_end_time.insert(component, end_t);
    }

    fn get_packet(&self, current_time: f64) -> String {
        if self.gps_timeout {
            return String::new(); // Simulate no data
        }

        // CSV Construction
        let uptime = ((current_time - self.start_time) * 1000.0) as u32;
        
        // Order: uptime, status, co2, ax, ay, az, gx, gy, gz, mx, my, mz ...
        // Using strict formatting
        
        let lat_e7 = (self.lat * 1e7) as i32;
        let lon_e7 = (self.lon * 1e7) as i32;

        format!(
            "ALL:{},0,{},{},{},{},0,0,0,0,0,0,3050,{},{},2800,{},{},{:.1},3,12,15,8,4,2,1,12,00,00,1,1,2026,4200,5,10,15,20,5000,{},{},{},0,0,{:.1},{:.1},0.0,0.0\n",
            uptime, self.co2,
            self.acc_x, self.acc_y, self.acc_z, // IMU
            self.temp_c, self.temp_c, // Temps
            lat_e7, lon_e7, self.alt, // GPS
            self.press_pa, self.temp_c, // Env
            self.rad_x100, // Rad
            self.alt, self.alt // Fusion
        )
    }

    fn get_gui_state(&self) -> PyResult<(f64, f64, f64, f64, f64, f64, f64)> {
        // Return tuple: (lat, lon, alt, press_hpa, acc_z_g, co2, rad_usv)
        let press_hpa = self.press_pa as f64 / 100.0;
        let rad_usv = self.rad_x100 as f64 / 100.0;
        
        Ok((self.lat, self.lon, self.alt, press_hpa, self.acc_z_g, self.co2 as f64, rad_usv))
    }
}

#[pymodule]
fn sim_core(_py: Python, m: &PyModule) -> PyResult<()> {
    m.add_class::<RustSimulator>()?;
    Ok(())
}
