#include <pebble.h>
#include "health.h"

#define DEBUG 0

typedef enum {
  AppKeyCurrentAverage = 0,
  AppKeyDailyAverage,
  AppKeyCurrentSteps
} AppKey;

typedef enum {
  AverageTypeCurrent = 0,
  AverageTypeDaily
} AverageType;

static int health_updated = false;

static int s_current_steps, s_daily_average, s_current_average;
static char s_current_steps_buffer[14];


int is_health_updated( void )
{
  if ( health_updated )
  {
	health_updated = false;  
    return true;		
  }
  return false;
}

static void health_handler(HealthEventType event, void *context) {
  if ( event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate )
  {
	if ( !health_updated )
	  health_updated = true;
  }
}

static void update_average(AverageType type) {
  // Start time is midnight
  const time_t start = time_start_of_today();

  time_t end = start;
  int steps = 0;
  switch(type) {
    case AverageTypeDaily:
      // One whole day
      end = start + SECONDS_PER_DAY;
      break;
    case AverageTypeCurrent:
      // Time from midnight to now
      end = start + (time(NULL) - time_start_of_today());
      break;
    default:
      if(DEBUG) APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown average type!");
      break;
  } 

  // Check the average data is available
  HealthServiceAccessibilityMask mask = health_service_metric_averaged_accessible(
                                HealthMetricStepCount, start, end, HealthServiceTimeScopeWeekly);
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    // Data is available, read it
    steps = (int)health_service_sum_averaged(HealthMetricStepCount, start, end, 
                                                                    HealthServiceTimeScopeWeekly);
  } else {
    if(DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "No data available for daily average");
  }

  // Store the calculated value
  switch(type) {
    case AverageTypeDaily:
      s_daily_average = steps;
      //persist_write_int(AppKeyDailyAverage, s_daily_average);

      if(DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Daily average: %d", s_daily_average);
      break;
    case AverageTypeCurrent:
      s_current_average = steps;
      //persist_write_int(AppKeyCurrentAverage, s_current_average);

      if(DEBUG) APP_LOG(APP_LOG_LEVEL_DEBUG, "Current average: %d", s_current_average);
      break;
    default: break;  // Handled by previous switch
  }
}

void health_update_steps_buffer() {
  if ( 0 == s_current_steps )
  {
	  snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer), " " );
	  return;
  }
	
  int thousands = s_current_steps / 1000;
  int hundreds = s_current_steps % 1000;
  if(thousands > 0) {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer), "\U0001F495%d,%03d", thousands, hundreds);
  } else {
    snprintf(s_current_steps_buffer, sizeof(s_current_steps_buffer), "\U0001F495%d", hundreds);
  }
}

static void load_health_health_handler(void *context) {
  const time_t start = time_start_of_today();
  time_t end = start + SECONDS_PER_DAY;
  HealthServiceAccessibilityMask mask = health_service_metric_averaged_accessible(
                                HealthMetricStepCount, start, end, HealthServiceTimeScopeWeekly);
  if( 0 == ( mask & HealthServiceAccessibilityMaskAvailable ) ) {
    // Data is not available
	APP_LOG(APP_LOG_LEVEL_DEBUG, "health reload ended, no permission");
	return;
  }
	
  s_current_steps = health_service_sum_today(HealthMetricStepCount);
  //persist_write_int(AppKeyCurrentSteps, s_current_steps);
  update_average(AverageTypeDaily);
  update_average(AverageTypeCurrent);

  health_update_steps_buffer();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "health reload successfully");
}

void health_reload_averages( const int loadDataDelayTime ) {
  if ( loadDataDelayTime > 0 )
  {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "health reloading with delay");
  	app_timer_register(loadDataDelayTime, load_health_health_handler, NULL);
  }	
  else
  {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "health reloading without delay");
	load_health_health_handler( (void*)&loadDataDelayTime );
  }
}

void health_init() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "health init");
  // First time persist
  //if(!persist_exists(AppKeyCurrentSteps)) {
    s_current_steps = 0;
    s_current_average = 0;
    s_daily_average = 0;
  //} else {
  //  s_current_average = persist_read_int(AppKeyCurrentAverage);
  //  s_daily_average = persist_read_int(AppKeyDailyAverage);
  //  s_current_steps = persist_read_int(AppKeyCurrentSteps);
  //}
  health_update_steps_buffer();

  // Avoid half-second delay loading the app by delaying API read
  const int loadDataDelayTime = 500;
  health_reload_averages( loadDataDelayTime );
	
  health_service_events_subscribe(health_handler, NULL);	
}

void health_deinit() {
  health_service_events_unsubscribe();
}

int health_get_current_steps() {
  return s_current_steps;
}

int health_get_current_average() {
  return s_current_average;
}

int health_get_daily_average() {
  return s_daily_average;
}

void health_set_current_steps(int value) {
  s_current_steps = value;
}

void health_set_current_average(int value) {
  s_current_average = value;
}

void health_set_daily_average(int value) {
  s_daily_average = value;
}

char* health_get_current_steps_buffer() {
  return s_current_steps_buffer;
}