#include <stdio.h>
#include <math.h>
#include "unity.h"
#include "mock_hal.h"
#include "sensors.h"
#include "kalman.h"
#include "fdir.h"
#include "telemetry.h"

// --- Simulation State ---
typedef struct {
    float altitude; // m
    float velocity; // m/s (Positive = Up)
    float temp_C;   // Celsius
} SimState_t;

SimState_t sim_state = {0.0f, 0.0f, 25.0f};

// --- Helper Functions ---
void Sim_UpdatePhysics(float dt_s) {
    sim_state.altitude += sim_state.velocity * dt_s;
    
    // Simple Lapse Rate: -6.5C per 1000m
    sim_state.temp_C = 25.0f - (sim_state.altitude / 1000.0f) * 6.5f;
}

void Sim_UpdateSensors(void) {
    // Inject data into Mock HAL / Sensor Output
    // 1. Barometer (MS5611) via Mock I2C? 
    // Difficulty: Sensors_Read_Baro calls MS5611 driver which reads registers.
    // We mocked I2C Read in mock_hal.c to return `next_read_data`.
    // But Sensors_Read_All calls multiple sensors.
    // It is hard to sequence distinct I2C responses for different sensors in one loop using simple `next_read_data`.
    
    // ALTERNATIVE: Use "Weak" symbols or specific Mock injection functions in `sensors.c` if possible?
    // OR: Since we enabled REAL HAL calls, we rely on `HAL_I2C_Mem_Read`.
    // We can update `mock_hal.c` to have a smarter "Device Registry" to return different data based on Address.
    
    // Let's rely on a simpler approach: 
    // The `sensors.c` has `mock_altitude` logic inside it (lines 302-303) because I saw it in the file view earlier!
    // Wait, let me check `sensors.c` again.
    // Line 45: `static float mock_altitude = 100.0f;`
    // Line 302: `mock_altitude += 0.5f;`
    
    // Ah, `sensors.c` ALREADY has some internal mock logic for `Sensors_Read_All`.
    // I should probably use THAT or modifying it to be controllable.
    // Modify `sensors.c` to expose `Sensors_SetMockState(alt, temp)` would be cleanest for SITL.
}

// We need to modify sensors.c to accept external mock data injection 
// because I don't want to rely on the hardcoded `mock_altitude += 0.5f` loop.
// CHECK: sensors.c content.

void setUp(void) {
    MockI2C_ClearStats();
    // Reset System
}

void tearDown(void) {
}

extern void Sensors_SetMockData(float alt_m, float temp_c, float press_pa);

float AltitudeToPressure(float alt_m) {
    // International Standard Atmosphere (Troposphere)
    // P = P0 * (1 - L*h/T0)^(gM/RL)
    // T0 = 288.15 K, P0 = 101325 Pa
    // L = 0.0065 K/m
    const float P0 = 101325.0f;
    const float T0 = 288.15f;
    const float L = 0.0065f;
    const float g = 9.80665f;
    const float M = 0.0289644f;
    const float R = 8.3144598f;
    
    float exponent = (g * M) / (R * L);
    float base = 1.0f - (L * alt_m) / T0;
    return P0 * powf(base, exponent);
}

void test_full_mission_profile(void) {
    telemetry_payload_sensor_snapshot_t sensor_data;
    KalmanState_t kf_state;
    
    // 1. Init
    Sensors_Init();
    Kalman_Init();
    FDIR_Init();
    
    // 2. Ground Idle (0s - 5s)
    printf("\n[Phase 1] GROUND IDLE\n");
    sim_state.altitude = 100.0f;
    sim_state.velocity = 0.0f;
    
    for(int t=0; t<5; t++) {
        // Update Physics
        Sim_UpdatePhysics(1.0f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        // Loop
        Sensors_Read_All(&sensor_data);
        Kalman_Update(&sensor_data, &kf_state);
        FDIR_Update();
        MockHAL_AdvanceTick(1000);
        
        printf("T=%d Alt=%.1f KF=%.1f\n", t, sim_state.altitude, kf_state.altitude_m);
        TEST_ASSERT_FLOAT_WITHIN(10.0f, sim_state.altitude, kf_state.altitude_m);
    }
    
    // 3. Ascent (5s - 15s) -> 5m/s
    printf("\n[Phase 2] ASCENT (5m/s)\n");
    sim_state.velocity = 5.0f;
    for(int t=0; t<10; t++) {
        Sim_UpdatePhysics(1.0f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        Sensors_Read_All(&sensor_data);
        Kalman_Update(&sensor_data, &kf_state);
        FDIR_Update();
        MockHAL_AdvanceTick(1000);
        
        printf("T=%d Alt=%.1f KF=%.1f Vel=%.1f\n", t+5, sim_state.altitude, kf_state.altitude_m, kf_state.vertical_velocity_mps);
    }
    // Verify velocity tracking
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 5.0f, kf_state.vertical_velocity_mps);
    
    // 4. Burst & Descent (15s - 25s) -> -20m/s
    printf("\n[Phase 3] BURST & DESCENT (-20m/s)\n");
    sim_state.velocity = -20.0f; // Sudden drop
    for(int t=0; t<10; t++) {
        Sim_UpdatePhysics(1.0f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        Sensors_Read_All(&sensor_data);
        Kalman_Update(&sensor_data, &kf_state);
        FDIR_Update();
        MockHAL_AdvanceTick(1000); // 1s
        
        printf("T=%d Alt=%.1f KF=%.1f Vel=%.1f\n", t+15, sim_state.altitude, kf_state.altitude_m, kf_state.vertical_velocity_mps);
    }
    // Verify Descent Detection
    // KF should lag slightly but converge to negative velocity
    TEST_ASSERT_TRUE(kf_state.vertical_velocity_mps < -10.0f);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_full_mission_profile);
    return UnityEnd();
}
