#include "health.h"

static bool s_health_available;
static HealthValue s_steps;
static HealthValue s_average_steps;

static void health_handler(HealthEventType event, void *context) {
    HealthValue cur_steps = health_service_sum_today(HealthMetricStepCount);
    APP_LOG(APP_LOG_LEVEL_INFO, "Current step count: %d steps", (int)cur_steps);
    if((int)cur_steps != (int)s_steps)
    {
      s_steps = cur_steps;
      s_average_steps = health_service_sum_averaged(HealthMetricStepCount, 
                                                  time_start_of_today(), 
                                                  time(NULL), 
                                                  HealthServiceTimeScopeDaily);
    APP_LOG(APP_LOG_LEVEL_INFO, "Average step count: %d average steps", (int)s_average_steps);
    update_face_layer((int)s_steps, (int)s_average_steps);
    }
}

void health_init() {
  s_health_available = health_service_events_subscribe(health_handler, NULL);
  if(!s_health_available) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  }
}
 
bool health_is_available() {
  return s_health_available;
}

int health_get_metric_sum(HealthMetric metric) {
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
    time_start_of_today(), time(NULL));
  if(mask == HealthServiceAccessibilityMaskAvailable) {
    return (int)health_service_sum_today(metric);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
    return 0;
  }
}