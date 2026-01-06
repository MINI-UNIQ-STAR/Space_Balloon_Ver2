#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Altitude Kalman + innovation gating for FDIR.
//
// Design goal: minimize false positives.
// - Use conservative chi-square threshold for 1 DOF innovation (NIS = r^2 / S)
// - Require consecutive outliers before raising fault
// - Reflect sensor measurement uncertainty via R (variance)
//
// This service currently uses BARO altitude only (MS5611). It does not fuse IMU accel until
// attitude/gravity compensation is available (to avoid false positives).

void alt_kf_service_init(void);
void alt_kf_service_reset(void);
void alt_kf_service_tick(uint32_t now_ms);

// Returns the latest estimated altitude in meters (float) if the filter is initialized.
bool alt_kf_service_get_alt_m(float *out_alt_m);

// True if baro measurements are currently considered faulty (after debounce).
bool alt_kf_service_is_baro_fault_active(void);

#ifdef __cplusplus
}
#endif
