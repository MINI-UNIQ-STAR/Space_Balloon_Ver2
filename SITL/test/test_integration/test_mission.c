/**
 * @file test_mission.c
 * @brief 전체 임무 프로파일(Full Mission Profile) 시뮬레이션 테스트 (SITL)
 * @details 전체 비행 과정을 모사하여 소프트웨어 통합 검증
 *          - 물리 엔진 시뮬레이션 (고도, 속도, 온습도 등)
 *          - 단계 검증: 지상 대기 -> 상승(5m/s) -> 버스트 -> 하강(-20m/s)
 *          - 핵심 알고리즘(칼만 필터, 상태 추정)이 시뮬레이션 물리량을
 *            정확히 추적하는지 검증
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include <stdio.h>
#include <math.h>
#include "unity.h"
#include "mock_hal.h"
#include "app.h"
#include "sensors.h"
#include "sensors.h"
// #include "mock_sensors.h"

extern void MockSensors_ClearStats(void);

// --- 시뮬레이션 상태 ---
typedef struct {
    float altitude; /**< 고도 (m) */
    float velocity; /**< 수직 속도 (m/s) (양수=상승) */
    float temp_C;   /**< 온도 (Celsius) */
} SimState_t;

SimState_t sim_state = {0.0f, 0.0f, 25.0f};

// --- Mock BSP ---
// App_Init에서 BSP_Init을 호출하므로 Mock 구현 필요
void BSP_Init(void) {
    // Mock 초기화 (필요 시 구현)
}
void BSP_Sensor_PowerOn(void) {}
void BSP_Delay(uint32_t Delay) {}
uint32_t BSP_GetTick(void) { return 0; }
// 텔레메트리용 UART Stub
int32_t BSP_UART_Write(uint8_t *pData, uint16_t Len) { return 0; }
int32_t BSP_UART_Read(uint8_t *pData, uint16_t Len) { return 0; }

// --- Helper Functions ---

/** @brief 물리 엔진 업데이트 (선형 운동 및 환경 모델) */
void Sim_UpdatePhysics(float dt_s) {
    sim_state.altitude += sim_state.velocity * dt_s;
    
    // 단순 기온 감율 모델 적용: -6.5도 / 1000m
    sim_state.temp_C = 25.0f - (sim_state.altitude / 1000.0f) * 6.5f;
}

extern void Sensors_SetMockData(float alt_m, float temp_c, float press_pa);

/** @brief 고도(m)를 기압(Pa)으로 변환 (ISA 표준 대기 모델) */
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

/** @brief 전체 임무 프로파일 테스트 검증 */
void test_full_mission_profile(void) {
    // 1. 시스템 초기화
    App_Init();
    
    // 2. 지상 대기 (Ground Idle) 구간 (0s - 5s)
    printf("\n[Phase 1] 지상 대기 (GROUND IDLE)\n");
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
    // 초기화 및 정지 상태 검증 (추정 고도가 시뮬레이션 고도 100m 근처인지)
    TEST_ASSERT_FLOAT_WITHIN(10.0f, sim_state.altitude, telem_frame.payload.kf_alt_m);
    
    // 3. 상승 (Ascent) 구간 (5s - 15s): 5m/s 속도 상승
    printf("\n[Phase 2] 상승 (ASCENT) - 5m/s\n");
    sim_state.velocity = 5.0f;
    for(int t=0; t<500; t++) { // 10s at 50Hz
        Sim_UpdatePhysics(0.02f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        App_Loop();
        MockHAL_AdvanceTick(20);
    }
    printf("상승 종료: Sim=%.1f KF=%.1f Vel=%.1f\n", sim_state.altitude, telem_frame.payload.kf_alt_m, hkf.x[1]);
    // 칼만 필터가 상승 속도(5.0)를 잘 추정하는지 확인 (오차 2.0 이내)
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 5.0f, hkf.x[1]);
    
    // 4. 파열 및 하강 (Burst & Descent) 구간 (15s - 25s): -20m/s 급하강
    printf("\n[Phase 3] 파열 및 하강 (BURST & DESCENT) - -20m/s\n");
    sim_state.velocity = -20.0f;
    for(int t=0; t<500; t++) { // 10s
        Sim_UpdatePhysics(0.02f);
        float p = AltitudeToPressure(sim_state.altitude);
        Sensors_SetMockData(sim_state.altitude, sim_state.temp_C, p);
        
        App_Loop();
        MockHAL_AdvanceTick(20);
    }
    printf("하강 종료: Sim=%.1f KF=%.1f Vel=%.1f\n", sim_state.altitude, telem_frame.payload.kf_alt_m, hkf.x[1]);
    // 하강 속도가 충분히 음수인지(추락 감지) 확인
    TEST_ASSERT_TRUE(hkf.x[1] < -10.0f);
}

int main(void) {
    UnityBegin();
    RUN_TEST(test_full_mission_profile);
    return UnityEnd();
}
