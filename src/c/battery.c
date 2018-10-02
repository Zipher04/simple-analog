#include <pebble.h>
#include "battery.h"

static int s_battery_updated = false;

static int s_battery_level = 0;
static bool s_is_charging = false;

static char s_current_level_buffer[5] = "";

int is_battery_updated( void )
{
  if ( s_battery_updated )
  {
	s_battery_updated = false;  
    return true;		
  }
  return false;
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery update");
  s_battery_level = state.charge_percent;
  s_is_charging = state.is_charging;
  snprintf( s_current_level_buffer, sizeof( s_current_level_buffer ), "%d%%", s_battery_level );
  s_battery_updated = true;
}

//----------------------------------------------------------
//public
//----------------------------------------------------------
int battery_get_level()
{
	return s_battery_level;
}

char* battery_get_current_level_buffer()
{
	return s_current_level_buffer;
}

bool battery_is_charging()
{
	return s_is_charging;
}

void battery_init() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery init");
  
  battery_state_service_subscribe( battery_callback );
  battery_callback( battery_state_service_peek() );
}

void battery_deinit() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery deinit");
  battery_state_service_unsubscribe();
}