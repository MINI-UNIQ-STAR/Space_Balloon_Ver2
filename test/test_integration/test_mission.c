#include <stdio.h>
#include <math.h>
#include "unity.h"
#include "mock_hal.h"
#include "app.h"
#include "sensors.h"

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

extern void Sensors_SetMockData(float alt_m, float temp_c, float press_pa);

float AltitudeToPressure(float alt_m) {
    const float P0 = 101325.0f;
    const float T0 = 288.15f;
    const float L = 0.0065f;
    const float g = 9.80665f;
    const float M = 0.0289644f;
    const float R = 8.3144598f;
    
    float exponent = (g * M) / (R * L);
    float base = 1.0f - (L * alt_m) / T0;
    if (base <= 0) return 0.0f;
    return P0 * powf(base, exponent);
}

void setUp(void) {
    MockSensors_ClearStats();
}

void tearDown(void) {
}

void test_full_mission_profile(void) {
    // 1. Init System
    App_Init();
    
    // 2. Ground Idle (0s - 5s)
    printf("\n[Phase 1] GROUND IDLE\n");
    sim_state.altitude = 100.0f;
    sim_state.velocity = 0.0f;
    
    for(int i=0; i<250; i++) { // 250 * 20ms = 5s
        Sim_UpdatePhysics(0.02f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        App_Loop();
        MockHAL_AdvanceTick(20);
        
        if (i % 50 == 0) {
             printf("T=%.2fs Sim=%.1f BaroP=%u KF=%.1f\n", i*0.02f, sim_state.altitude, telem_frame.payload.ms5611_press_pa, telem_frame.payload.kf_alt_m);
        }
    }
    TEST_ASSERT_FLOAT_WITHIN(10.0f, sim_state.altitude, telem_frame.payload.kf_alt_m);
    
    // 3. Ascent (5s - 15s) -> 5m/s
    printf("\n[Phase 2] ASCENT (5m/s)\n");
    sim_state.velocity = 5.0f;
    for(int t=0; t<500; t++) { // 10s at 50Hz
        Sim_UpdatePhysics(0.02f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        App_Loop();
        MockHAL_AdvanceTick(20);
    }
    printf("End of Ascent: Sim=%.1f KF=%.1f Vel=%.1f\n", sim_state.altitude, telem_frame.payload.kf_alt_m, hkf.x[1]);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 5.0f, hkf.x[1]);
    
    // 4. Burst & Descent (15s - 25s) -> -20m/s
    printf("\n[Phase 3] BURST & DESCENT (-20m/s)\n");
    sim_state.velocity = -20.0f;
    for(int t=0; t<500; t++) { // 10s
        Sim_UpdatePhysics(0.02f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        App_Loop();
        MockHAL_AdvanceTick(20);
    }
    printf("End of Descent: Sim=%.1f KF=%.1f Vel=%.1f\n", sim_state.altitude, telem_frame.payload.kf_alt_m, hkf.x[1]);
    TEST_ASSERT_TRUE(hkf.x[1] < -10.0f);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_full_mission_profile);
    return UnityEnd();
}
