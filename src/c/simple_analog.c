#include "simple_analog.h"

#include "pebble.h"
#include "health.h"
#include "battery.h"

#define COLOR_PURPLE GColorFromRGB(137,92,129)
#define COLOR_SILVER GColorFromRGB(167,158,151)

#define COLOR_MARKS_1 GColorShockingPink
#define COLOR_MARKS_2 GColorShockingPink
#define COLOR_HAND GColorShockingPink
#define COLOR_DATE GColorBlack

static Window *s_window;
static Layer *s_layer_date;
static TextLayer *s_text_layer_weekday, *s_text_layer_day, *s_text_layer_step;
static TextLayer *s_text_layer_battery;
static TextLayer *s_time_layer;
static TextLayer *s_small_time_layer;
static TextLayer *s_weather_layer;

static char s_num_buffer[4], s_day_buffer[6];
static BitmapLayer *s_bitmap_layer_background, *s_bitmap_layer_background_base;
static GBitmap *s_background_bitmap = 0, *s_background_bitmap_base = 0, *s_background_bitmap_transparent = 0;

static GFont s_time_font;

// Store weather information
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];

static void prv_unobstructed_did_change(void *context) {
	Layer *window_layer = window_get_root_layer(s_window);

	// Get the full size of the screen
	GRect full_bounds = layer_get_bounds(window_layer);
	// Get the total available screen real-estate
	GRect bounds = layer_get_unobstructed_bounds(window_layer);
	if ( grect_equal(&full_bounds, &bounds) ) {
		// Screen is no longer obstructed, show the date
		layer_set_hidden(text_layer_get_layer(s_small_time_layer), true);
		layer_set_hidden(text_layer_get_layer(s_time_layer), false);
		layer_set_hidden(text_layer_get_layer(s_text_layer_weekday), false);
		layer_set_hidden(text_layer_get_layer(s_text_layer_day), false);
		layer_set_hidden(text_layer_get_layer(s_text_layer_step), false);
	}
	else
	{
		layer_set_hidden(text_layer_get_layer(s_small_time_layer), false);
		layer_set_hidden(text_layer_get_layer(s_time_layer), true);
		layer_set_hidden(text_layer_get_layer(s_text_layer_weekday), true);
		layer_set_hidden(text_layer_get_layer(s_text_layer_day), true);
		layer_set_hidden(text_layer_get_layer(s_text_layer_step), true);
	}
}

static void update_time( void )
{
  // Get a tm structure
  time_t temp_time = time(NULL);
  struct tm *tick_time = localtime(&temp_time);

  // Write the current hours and minutes into a buffer
  static char s_time_buffer[8];
  strftime( s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text( s_time_layer, s_time_buffer );
  text_layer_set_text( s_small_time_layer, s_time_buffer );
}

static void choose_background_bitmap( struct tm *tick_time ) {
	static GBitmap *new_background_bitmap;
	if ( 0 == tick_time )
	{
		time_t now = time(NULL);
		tick_time = localtime(&now);
	}
	bool usingBase = false;
	if ( tick_time->tm_mon == 11 && tick_time->tm_mday == 25 )
	{
		usingBase = true;
		new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_TRANSPARENT );
	}
	else if ( tick_time->tm_wday == 0 || tick_time->tm_wday == 6 )	//sunday and saturday
	{
		if( tick_time->tm_hour <= 8 )
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_HolidaySleep );
		else
		{
			usingBase = true;
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_TRANSPARENT );
		}
	}
	else
	{
		switch ( tick_time->tm_hour )
		{
		case 6:
			if ( tick_time->tm_min < 30 )
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_0600 );
			else
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_0630 );
			break;
		case 7:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_0700 );
			break;
		case 8:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_0800 );
			break;
		case 10:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1000 );
			break;
		case 11:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1100 );
			break;
		case 12:
			if ( tick_time->tm_min < 30 )
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1200 );
			else
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1230 );
			break;
		case 13:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1300 );
			break;
		case 15:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1500 );
			break;
		case 17:
			if ( tick_time->tm_min < 30 )
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1700 );
			else
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1730 );
			break;
		case 18:
			if ( tick_time->tm_min < 30 )
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1800 );
			else
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1830 );
			break;
		case 19:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1900 );
			break;
		case 20:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_2000 );
			break;
		case 22:
			//if ( tick_time->tm_min < 30 )
				new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_2200 );
			//else
			//	new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_2230 );
			break;
		case 23:
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_2300 );
			break;
		default:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_main );
			break;
		}
	}
	if ( NULL == new_background_bitmap )
	{
		text_layer_set_text(s_text_layer_weekday, "?");
		return;
	}
	if ( usingBase )
		bitmap_layer_set_bitmap(s_bitmap_layer_background_base, s_background_bitmap_base);
	else
		bitmap_layer_set_bitmap(s_bitmap_layer_background_base, s_background_bitmap_transparent);
	bitmap_layer_set_bitmap(s_bitmap_layer_background, new_background_bitmap);
	if ( 0 != s_background_bitmap )
	gbitmap_destroy(s_background_bitmap);
	s_background_bitmap = new_background_bitmap;
}

static void date_update_proc(Layer *layer, GContext *ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
	text_layer_set_text(s_text_layer_weekday, s_day_buffer);

	strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
	text_layer_set_text(s_text_layer_day, s_num_buffer);
}

static void tick_service_callback(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(window_get_root_layer(s_window));
	if ( (units_changed & DAY_UNIT) != 0 )
	{
		layer_mark_dirty( s_layer_date );
	}
	if ( (units_changed & HOUR_UNIT) != 0 )
	{
		choose_background_bitmap( tick_time );
	}
	if ( (units_changed & MINUTE_UNIT) != 0 )
	{
		update_time();
		if ( tick_time->tm_min == 30 )
		{
			choose_background_bitmap( tick_time );
		}
		if ( is_health_updated() )
		{
	  	health_reload_averages(0);
			text_layer_set_text( s_text_layer_step, health_get_current_steps_buffer() );
		}
	}
	if ( is_battery_updated() )
	{
		if ( battery_is_charging() )
			text_layer_set_text( s_text_layer_battery, "\U0001F606" );
		else
			text_layer_set_text( s_text_layer_battery, battery_get_current_level_buffer() );
	}
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	health_init();
	battery_init();
	
	// Create background_base image
	s_bitmap_layer_background_base = bitmap_layer_create( GRect(0, 0, 144, 168) );
	bitmap_layer_set_background_color( s_bitmap_layer_background_base, GColorClear );
	bitmap_layer_set_compositing_mode( s_bitmap_layer_background_base, GCompOpSet );
	
	time_t now = time(NULL);
	struct tm* tick_time = localtime(&now);

	if ( tick_time->tm_mon == 11 && tick_time->tm_mday == 25 )
	{
		s_background_bitmap_base = gbitmap_create_with_resource( RESOURCE_ID_XMAS );
	}
	else if ( tick_time->tm_wday == 0 )
	{
		s_background_bitmap_base = gbitmap_create_with_resource( RESOURCE_ID_BUBBLE );
	}
	else
	{
		s_background_bitmap_base = gbitmap_create_with_resource( RESOURCE_ID_HOLIDAY );
	}
	s_background_bitmap_transparent = gbitmap_create_with_resource( RESOURCE_ID_TRANSPARENT );
	layer_add_child( window_layer, bitmap_layer_get_layer(s_bitmap_layer_background_base));
	
	// Create background image
	s_bitmap_layer_background = bitmap_layer_create( GRect(16, 0, 112, 122) );
	bitmap_layer_set_background_color( s_bitmap_layer_background, GColorClear );
	bitmap_layer_set_compositing_mode( s_bitmap_layer_background, GCompOpSet );
	choose_background_bitmap(0);
	layer_add_child( window_layer, bitmap_layer_get_layer(s_bitmap_layer_background));
	
	// Create small time layer
	s_small_time_layer = text_layer_create( GRect( 0, 0, bounds.size.w, 18 ) );
	text_layer_set_background_color( s_small_time_layer, GColorWhite );
	text_layer_set_text_color( s_small_time_layer, GColorBlack );
	text_layer_set_font( s_small_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD) );
	text_layer_set_text_alignment( s_small_time_layer, GTextAlignmentCenter );
	layer_add_child( window_layer, text_layer_get_layer(s_small_time_layer) );
	layer_set_hidden( text_layer_get_layer(s_small_time_layer), true );
	
	// Time font
	s_time_font = fonts_load_custom_font( resource_get_handle(RESOURCE_ID_FONT_COMIC_42));
	
	// Time layers
	s_time_layer = text_layer_create( GRect( 0, 120, bounds.size.w, 48 ) );
	text_layer_set_background_color( s_time_layer, GColorClear );
	text_layer_set_text_color( s_time_layer, GColorDukeBlue );
	text_layer_set_font( s_time_layer, s_time_font );
	text_layer_set_text_alignment( s_time_layer, GTextAlignmentCenter );
	layer_add_child( window_layer, text_layer_get_layer(s_time_layer) );
	update_time();
		
	// Create date layers
	s_layer_date = layer_create(bounds);
	layer_set_update_proc(s_layer_date, date_update_proc);
	layer_add_child(window_layer, s_layer_date);

	s_text_layer_weekday = text_layer_create(PBL_IF_ROUND_ELSE(
	GRect(63, 114, 27, 20),
	GRect(22, 107, 36, 18)));
	text_layer_set_text(s_text_layer_weekday, s_day_buffer);
	text_layer_set_background_color(s_text_layer_weekday, GColorClear);
	text_layer_set_text_color(s_text_layer_weekday, GColorDukeBlue );
	text_layer_set_font(s_text_layer_weekday, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(s_layer_date, text_layer_get_layer(s_text_layer_weekday));

	s_text_layer_day = text_layer_create(PBL_IF_ROUND_ELSE(
	GRect(90, 114, 18, 20),
	GRect(50, 107, 36, 18)));
	text_layer_set_text(s_text_layer_day, s_num_buffer);
	text_layer_set_background_color(s_text_layer_day, GColorClear);
	text_layer_set_text_color(s_text_layer_day, GColorDukeBlue);
	text_layer_set_font(s_text_layer_day, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(s_layer_date, text_layer_get_layer(s_text_layer_day));
	
	//step layer
	s_text_layer_step = text_layer_create( GRect(65, 107, 64, 18) );
	text_layer_set_text( s_text_layer_step, health_get_current_steps_buffer() );
	text_layer_set_text_alignment( s_text_layer_step, GTextAlignmentCenter );
	//static char test[] = "\U0001F4951,234";
	//text_layer_set_text( s_text_layer_step, test );
	text_layer_set_background_color(s_text_layer_step, GColorClear);
	text_layer_set_text_color( s_text_layer_step, GColorDukeBlue );
	text_layer_set_font( s_text_layer_step, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child( window_layer, text_layer_get_layer(s_text_layer_step));
	health_reload_averages(0);
	
	//battery layer
	s_text_layer_battery = text_layer_create( GRect(0, 0, bounds.size.w, 18) );
	text_layer_set_text_alignment( s_text_layer_battery, GTextAlignmentRight );
	text_layer_set_text( s_text_layer_battery, "10" );
	text_layer_set_background_color( s_text_layer_battery, GColorClear);
	text_layer_set_text_color( s_text_layer_battery, GColorBlack );
	text_layer_set_font( s_text_layer_battery, fonts_get_system_font( FONT_KEY_GOTHIC_18 ));
	layer_add_child( window_layer, text_layer_get_layer(s_text_layer_battery) );
	
	//weather
	s_weather_layer = text_layer_create( GRect(0, 0, bounds.size.w, 25) );
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorBlack);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
	//text_layer_set_text(s_weather_layer, "Loading...");
	text_layer_set_font(s_weather_layer, fonts_get_system_font( FONT_KEY_GOTHIC_18 ) );
	layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
	
	UnobstructedAreaHandlers handlers = {
		.did_change = prv_unobstructed_did_change
	};
	unobstructed_area_service_subscribe(handlers, NULL);
	prv_unobstructed_did_change( 0 );
}

static void window_unload(Window *window) {
	
	unobstructed_area_service_unsubscribe();

	text_layer_destroy(s_weather_layer);
	
	text_layer_destroy(s_text_layer_step);
	text_layer_destroy(s_text_layer_day);
	text_layer_destroy(s_text_layer_weekday);
	layer_destroy(s_layer_date);
	
	text_layer_destroy(s_time_layer);
	fonts_unload_custom_font(s_time_font);
	
	bitmap_layer_destroy(s_bitmap_layer_background);
	gbitmap_destroy(s_background_bitmap);
	
	bitmap_layer_destroy(s_bitmap_layer_background_base);
	gbitmap_destroy(s_background_bitmap_transparent);
	gbitmap_destroy(s_background_bitmap_base);
	
	battery_deinit();
	health_deinit();
	
}
//appmessage
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	// Read tuples for data
	Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
	Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

	// If all data is available, use it
	if(temp_tuple && conditions_tuple) {
	  snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
	  snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
	}
	// Assemble full string and display
	snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
	text_layer_set_text(s_weather_layer, weather_layer_buffer);
}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
	app_message_register_inbox_received(inbox_received_callback);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

	const int inbox_size = 128;
	const int outbox_size = 128;
	app_message_open(inbox_size, outbox_size);
	
	s_window = window_create();
	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_stack_push(s_window, true);

	s_day_buffer[0] = '\0';
	s_num_buffer[0] = '\0';

	Layer *window_layer = window_get_root_layer(s_window);
	GRect bounds = layer_get_bounds(window_layer);

	tick_timer_service_subscribe(MINUTE_UNIT, tick_service_callback);
}

static void deinit() {
	/* 
	gpath_destroy(s_hour_arrow);
	gpath_destroy(s_minute_arrow);
	*/
	tick_timer_service_unsubscribe();

	window_destroy(s_window);
}

int main() {
	init();
	app_event_loop();
	deinit();
}
