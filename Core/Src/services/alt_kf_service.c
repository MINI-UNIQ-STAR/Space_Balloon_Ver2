#include "services/alt_kf_service.h"

#include "services/ms5611_service.h"
#include "services/gps_service.h"

#include "stm32g4xx_hal.h"

// Tuning knobs (conservative defaults; override via build_flags if needed)
#ifndef ALT_KF_R_SIGMA_Z_M
#define ALT_KF_R_SIGMA_Z_M 3.0f
#endif

#ifndef ALT_KF_Q_SIGMA_A_MPS2
#define ALT_KF_Q_SIGMA_A_MPS2 5.0f
#endif

// Chi-square threshold for 1 DOF (innovation). Conservative to reduce false positives.
#ifndef ALT_KF_NIS_THRESHOLD
#define ALT_KF_NIS_THRESHOLD 16.0f
#endif

// Require N consecutive outliers before flagging fault.
#ifndef ALT_KF_OUTLIER_CONSECUTIVE
#define ALT_KF_OUTLIER_CONSECUTIVE 5u
#endif

// Service tick period (does prediction at this cadence).
#ifndef ALT_KF_TICK_PERIOD_MS
#define ALT_KF_TICK_PERIOD_MS 20u
#endif

typedef struct {
	float h; // altitude (m)
	float v; // vertical speed (m/s)
	float P00;
	float P01;
	float P10;
	float P11;
	bool initialized;
} kf_state_t;

static kf_state_t s_kf;
static uint32_t s_next_tick_ms;
static uint32_t s_last_predict_ms;
static uint32_t s_last_meas_update_ms;
static uint32_t s_last_gps_update_ms;
static uint8_t s_outlier_streak;
static bool s_baro_fault_active;

static float clamp_dt(float dt)
{
	if (dt < 0.0f) {
		return 0.0f;
	}
	// Clamp to avoid exploding covariance when the scheduler stalls.
	if (dt > 0.5f) {
		return 0.5f;
	}
	return dt;
}

static void kf_init(float h0)
{
	s_kf.h = h0;
	s_kf.v = 0.0f;

	// Initial covariance: fairly large uncertainty (conservative).
	s_kf.P00 = 25.0f; // (5m)^2
	s_kf.P01 = 0.0f;
	s_kf.P10 = 0.0f;
	s_kf.P11 = 25.0f; // (5 m/s)^2

	s_kf.initialized = true;
}

static void kf_predict(float dt)
{
	// State transition: h += v*dt, v = v
	const float h = s_kf.h;
	const float v = s_kf.v;
	s_kf.h = h + v * dt;
	s_kf.v = v;

	// Covariance predict: P = F P F^T + Q
	// F = [[1, dt],[0,1]]
	const float P00 = s_kf.P00;
	const float P01 = s_kf.P01;
	const float P10 = s_kf.P10;
	const float P11 = s_kf.P11;

	// Acceleration noise model (continuous white noise acceleration) mapped into (h,v)
	const float sigma_a = (float)ALT_KF_Q_SIGMA_A_MPS2;
	const float sigma_a2 = sigma_a * sigma_a;
	const float dt2 = dt * dt;
	const float dt3 = dt2 * dt;
	const float dt4 = dt2 * dt2;

	const float Q00 = 0.25f * dt4 * sigma_a2;
	const float Q01 = 0.5f * dt3 * sigma_a2;
	const float Q10 = 0.5f * dt3 * sigma_a2;
	const float Q11 = dt2 * sigma_a2;

	// FPF^T
	const float FP00 = P00 + dt * P10;
	const float FP01 = P01 + dt * P11;
	const float FP10 = P10;
	const float FP11 = P11;

	const float Pn00 = FP00 + dt * FP01;
	const float Pn01 = FP01;
	const float Pn10 = FP10 + dt * FP11;
	const float Pn11 = FP11;

	s_kf.P00 = Pn00 + Q00;
	s_kf.P01 = Pn01 + Q01;
	s_kf.P10 = Pn10 + Q10;
	s_kf.P11 = Pn11 + Q11;
}

static bool kf_update_baro(float z)
{
	// Measurement: z = h + noise
	const float R = (float)ALT_KF_R_SIGMA_Z_M * (float)ALT_KF_R_SIGMA_Z_M;

	const float r = z - s_kf.h;
	const float S = s_kf.P00 + R;
	if (S <= 1e-9f) {
		return false;
	}

	const float nis = (r * r) / S;
	if (nis > (float)ALT_KF_NIS_THRESHOLD) {
		return false; // outlier -> caller decides debounce + whether to skip update
	}

	// Kalman gain K = P H^T S^-1, with H = [1,0]
	const float K0 = s_kf.P00 / S;
	const float K1 = s_kf.P10 / S;

	// State update
	s_kf.h = s_kf.h + K0 * r;
	s_kf.v = s_kf.v + K1 * r;

	// Joseph form for numerical stability (still cheap for 2x2)
	// P = (I-KH) P (I-KH)^T + K R K^T
	const float I_KH00 = 1.0f - K0;
	const float I_KH01 = 0.0f;
	const float I_KH10 = -K1;
	const float I_KH11 = 1.0f;

	const float P00 = s_kf.P00;
	const float P01 = s_kf.P01;
	const float P10 = s_kf.P10;
	const float P11 = s_kf.P11;

	const float A00 = I_KH00 * P00 + I_KH01 * P10;
	const float A01 = I_KH00 * P01 + I_KH01 * P11;
	const float A10 = I_KH10 * P00 + I_KH11 * P10;
	const float A11 = I_KH10 * P01 + I_KH11 * P11;

	const float Pn00 = A00 * I_KH00 + A01 * I_KH01;
	const float Pn01 = A00 * I_KH10 + A01 * I_KH11;
	const float Pn10 = A10 * I_KH00 + A11 * I_KH01;
	const float Pn11 = A10 * I_KH10 + A11 * I_KH11;

	s_kf.P00 = Pn00 + (K0 * R * K0);
	s_kf.P01 = Pn01 + (K0 * R * K1);
	s_kf.P10 = Pn10 + (K1 * R * K0);
	s_kf.P11 = Pn11 + (K1 * R * K1);

	return true;
}

static bool kf_update_gps(float z, float sigma_z)
{
	// Measurement: z = h + noise
	const float R = sigma_z * sigma_z;

	const float r = z - s_kf.h;
	const float S = s_kf.P00 + R;
	if (S <= 1e-9f) {
		return false;
	}

	const float nis = (r * r) / S;
	if (nis > (float)ALT_KF_NIS_THRESHOLD) {
		return false; // outlier
	}

	// Kalman gain K = P H^T S^-1, with H = [1,0]
	const float K0 = s_kf.P00 / S;
	const float K1 = s_kf.P10 / S;

	// State update
	s_kf.h = s_kf.h + K0 * r;
	s_kf.v = s_kf.v + K1 * r;

	// Joseph form
	const float I_KH00 = 1.0f - K0;
	const float I_KH01 = 0.0f;
	const float I_KH10 = -K1;
	const float I_KH11 = 1.0f;

	const float P00 = s_kf.P00;
	const float P01 = s_kf.P01;
	const float P10 = s_kf.P10;
	const float P11 = s_kf.P11;

	const float A00 = I_KH00 * P00 + I_KH01 * P10;
	const float A01 = I_KH00 * P01 + I_KH01 * P11;
	const float A10 = I_KH10 * P00 + I_KH11 * P10;
	const float A11 = I_KH10 * P01 + I_KH11 * P11;

	const float Pn00 = A00 * I_KH00 + A01 * I_KH01;
	const float Pn01 = A00 * I_KH10 + A01 * I_KH11;
	const float Pn10 = A10 * I_KH00 + A11 * I_KH01;
	const float Pn11 = A10 * I_KH10 + A11 * I_KH11;

	s_kf.P00 = Pn00 + (K0 * R * K0);
	s_kf.P01 = Pn01 + (K0 * R * K1);
	s_kf.P10 = Pn10 + (K1 * R * K0);
	s_kf.P11 = Pn11 + (K1 * R * K1);

	return true;
}

void alt_kf_service_init(void)
{
	s_kf = (kf_state_t){0};
	s_next_tick_ms = HAL_GetTick();
	s_last_predict_ms = 0;
	s_last_meas_update_ms = 0;
	s_last_gps_update_ms = 0;
	s_outlier_streak = 0;
	s_baro_fault_active = false;
}

void alt_kf_service_reset(void)
{
	alt_kf_service_init();
}

void alt_kf_service_tick(uint32_t now_ms)
{
	if ((int32_t)(now_ms - s_next_tick_ms) < 0) {
		return;
	}
	s_next_tick_ms = now_ms + (uint32_t)ALT_KF_TICK_PERIOD_MS;

	// Predict step
	if (s_last_predict_ms == 0u) {
		s_last_predict_ms = now_ms;
	} else {
		const float dt = clamp_dt((float)(now_ms - s_last_predict_ms) / 1000.0f);
		s_last_predict_ms = now_ms;
		if (s_kf.initialized) {
			kf_predict(dt);
		}
	}

	// Measurement update (Barometer)
	uint32_t baro_ms = 0;
	if (ms5611_service_get_last_update_ms(&baro_ms) && (baro_ms != s_last_meas_update_ms)) {
		s_last_meas_update_ms = baro_ms;

		int32_t t_c_x100 = 0;
		uint32_t press_pa = 0;
		int32_t alt_m = 0;
		if (ms5611_service_get_last(&t_c_x100, &press_pa, &alt_m)) {
			const float z = (float)alt_m;
			if (!s_kf.initialized) {
				kf_init(z);
				s_outlier_streak = 0;
				s_baro_fault_active = false;
				// If we just initialized, we can also check GPS, but let's wait for next tick.
				return;
			}

			// Gating + debounce
			if (kf_update_baro(z)) {
				s_outlier_streak = 0;
				s_baro_fault_active = false;
			} else {
				if (s_outlier_streak < 0xFFu) {
					s_outlier_streak++;
				}
				if (s_outlier_streak >= (uint8_t)ALT_KF_OUTLIER_CONSECUTIVE) {
					s_baro_fault_active = true;
				}
			}
		}
	}

	// Measurement update (GPS)
	uint32_t gps_ms = 0;
	if (gps_service_get_last_update_ms(&gps_ms) && (gps_ms != s_last_gps_update_ms)) {
		s_last_gps_update_ms = gps_ms;

		nmea_gps_state_t gps_state;
		if (gps_service_get_state(&gps_state) && gps_state.has_fix) {
			// GPS Altitude is in mm, convert to m
			float z_gps = (float)gps_state.alt_mm / 1000.0f;
			
			// Calculate Sigma based on HDOP
			// UERE (User Equivalent Range Error) ~ 5.0m
			// Sigma = HDOP * UERE
			float sigma = 10.0f; // Default conservative
			if (gps_state.hdop_x100 > 0) {
				sigma = ((float)gps_state.hdop_x100 / 100.0f) * 5.0f;
			}
			
			// If filter not initialized (e.g. Baro failed), init with GPS
			if (!s_kf.initialized) {
				kf_init(z_gps);
				return;
			}

			// Update KF with GPS
			// We don't track GPS outliers strictly here, but kf_update_gps has gating.
			kf_update_gps(z_gps, sigma);
		}
	}
}

bool alt_kf_service_get_alt_m(float *out_alt_m)
{
	if ((out_alt_m == NULL) || !s_kf.initialized) {
		return false;
	}
	*out_alt_m = s_kf.h;
	return true;
}

bool alt_kf_service_is_baro_fault_active(void)
{
	return s_baro_fault_active;
}
