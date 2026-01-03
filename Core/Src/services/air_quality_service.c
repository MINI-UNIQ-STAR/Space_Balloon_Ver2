#include "services/air_quality_service.h"

#include "services/pms3003_service.h"

void air_quality_service_init(void)
{
	pms3003_service_init();
}

void air_quality_service_tick(uint32_t now_ms)
{
	pms3003_service_tick(now_ms);
}

bool air_quality_get_pm(pms_reading_t *out)
{
	return pms3003_get_reading(out);
}
