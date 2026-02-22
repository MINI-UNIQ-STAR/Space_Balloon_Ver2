/**
 * @file actuators_zephyr.c
 * @brief 액추에이터 제어 - Zephyr 포팅 (스텁)
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(actuators, LOG_LEVEL_INF);

/* 액추에이터 초기화 */
void Actuators_Init(void) {
    LOG_INF("Actuators initialized (stub)");
}

/* 히터 제어 */
void Actuators_SetHeater(float duty_percent) {
    LOG_DBG("Heater duty: %.1f%%", duty_percent);
}

/* 펌프 제어 */
void Actuators_SetPump(bool enable) {
    LOG_DBG("Pump: %s", enable ? "ON" : "OFF");
}

/* 액추에이터 처리 */
void Actuators_Process(void) {
    /* Stub */
}
