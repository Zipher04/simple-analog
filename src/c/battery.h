#pragma once

void battery_init();
void battery_deinit();

int battery_get_level();
char* battery_get_current_level_buffer();

//return 1 for charging, else 0
bool battery_is_charging();

int is_battery_updated( void );