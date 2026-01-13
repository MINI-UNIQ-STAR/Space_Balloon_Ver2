/**
 * @file test_main.c
 * @brief PID 제어기 알고리즘 단위 테스트 (SITL)
 * @details 임베디드용 PID 제어 라이브러리의 동작 검증
 *          - P, I, D 제어항 개별 동작 및 통합 동작 테스트
 *          - 출력 제한(Clamping) 기능
 *          - 적분 누적 방지(Anti-windup) 로직 검증
 * @author Hyeonsu Park
 * @date 2026-01-13
 * @version 1.0
 */

#include <stdio.h>
#include "unity.h"
#include "pid.h"

// pid.c 내부 함수 원형 (주석으로 대신함)
// void PID_Init(PID_HandleTypeDef *hpid, float Kp, float Ki, float Kd, float MaxOutput);
// float PID_Update(PID_HandleTypeDef *hpid, float measurement, float dt);

void setUp(void) {
}

void tearDown(void) {
}

/** @brief PID 초기화 함수 동작 및 파라미터 저장 검증 */
void test_pid_init(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 1.0f, 2.0f, 3.0f, 100.0f);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, hpid.Kp);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, hpid.Ki);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, hpid.Kd);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 100.0f, hpid.MaxOutput);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hpid.IntegratedError);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, hpid.LastError);
}

/** @brief P (비례) 제어항 동작 테스트 */
void test_pid_p_term(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 2.0f, 0.0f, 0.0f, 100.0f); // Kp=2
    
    hpid.Target = 10.0f;
    float meas = 5.0f; // 오차 = 5
    float dt = 1.0f;
    
    // 출력 = Kp * 오차 = 2 * 5 = 10
    float output = PID_Update(&hpid, meas, dt);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 10.0f, output);
}

/** @brief I (적분) 제어항 누적 동작 테스트 */
void test_pid_i_term(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 0.0f, 1.0f, 0.0f, 100.0f); // Ki=1
    hpid.Target = 10.0f;
    
    // 단계 1: 오차 = 2 (측정 8), dt = 1 -> 누적 오차 += 2
    PID_Update(&hpid, 8.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, hpid.IntegratedError); 
    
    // 단계 2: 오차 = 2, dt = 1 -> 누적 오차 += 2 -> 총 4
    float output = PID_Update(&hpid, 8.0f, 1.0f);
    
    // 출력 = Ki * 누적 오차 = 1 * 4 = 4
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, output);
}

/** @brief D (미분) 제어항 동작 테스트 */
void test_pid_d_term(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 0.0f, 0.0f, 1.0f, 100.0f); // Kd=1
    hpid.Target = 10.0f;
    
    // 단계 1: 측정=5, 오차=5, 이전 오차=0 (초기값)
    // 미분항 = (5 - 0) / 1 = 5
    // 출력 = 1 * 5 = 5
    float out1 = PID_Update(&hpid, 5.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, out1);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, hpid.LastError);
    
    // 단계 2: 측정=8, 오차=2
    // 미분항 = (2 - 5) / 1 = -3
    // 출력 = 1 * -3 = -3 -> 0으로 제한 (히터는 음수 출력 불가, 0으로 클램핑)
    float out2 = PID_Update(&hpid, 8.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, out2);
}

/** @brief 출력 제한(Clamping) 기능 테스트 (0 ~ Max) */
void test_pid_clamping(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 10.0f, 0.0f, 0.0f, 50.0f); // Max 50
    hpid.Target = 100.0f;
    
    // 케이스 1: 큰 양의 오차
    // 오차 = 100, 계산된 출력 = 1000 -> 50으로 제한
    float out1 = PID_Update(&hpid, 0.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 50.0f, out1);
    
    // 케이스 2: 음수 오차
    // 오차 = -50 (측정 150), 계산된 출력 = -500 -> 0으로 제한 (히터 로직)
    float out2 = PID_Update(&hpid, 150.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, out2);
}

/** @brief 적분 누적 방지(Anti-windup) 테스트 */
void test_pid_anti_windup(void) {
    PID_HandleTypeDef hpid;
    PID_Init(&hpid, 100.0f, 1.0f, 0.0f, 50.0f); // Kp=100, Max=50
    hpid.Target = 100.0f;
    
    // 단계 1: 측정=0 -> 오차=100 -> P항=10000 -> 50으로 포화됨!
    // 출력이 포화 상태(Max 초과)이므로, I항이 더 이상 누적되지 않아야 함
    PID_Update(&hpid, 0.0f, 1.0f);
    float ie1 = hpid.IntegratedError;
    
    // 단계 2: 여전히 포화 상태, I항 누적 금지 확인
    PID_Update(&hpid, 0.0f, 1.0f);
    float ie2 = hpid.IntegratedError;
    
    // Anti-windup 검증: 적분 오차가 증가하지 않았어야 함
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, ie1, ie2);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, ie1); // 초기값 0 유지
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_pid_init);
    RUN_TEST(test_pid_p_term);
    RUN_TEST(test_pid_i_term);
    RUN_TEST(test_pid_d_term);
    RUN_TEST(test_pid_clamping);
    RUN_TEST(test_pid_anti_windup);
    
    return UNITY_END();
}
