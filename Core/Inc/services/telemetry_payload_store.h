#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "services/telemetry_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shared telemetry payload store protected by a mutex.
// Writer tasks update subsets of fields; the realtime sender reads a consistent snapshot.

void telemetry_payload_store_init(void);

// Writer-side locking (priority inheritance mutex).
// Keep the critical section short (copy/assign only).
bool telemetry_payload_store_write_lock(uint32_t timeout_ms);
void telemetry_payload_store_write_unlock(void);
telemetry_payload_sensor_snapshot_t *telemetry_payload_store_write_ptr_unsafe(void);

// Reader-side helper: copies a consistent snapshot. Returns false if it couldn't lock.
bool telemetry_payload_store_read_copy(telemetry_payload_sensor_snapshot_t *out, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
