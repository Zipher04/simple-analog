#include "simple_analog.h"

#include "pebble.h"
#include "health.h"

#define COLOR_PURPLE GColorFromRGB(137,92,129)
#define COLOR_SILVER GColorFromRGB(167,158,151)

#define COLOR_MARKS_1 GColorShockingPink
#define COLOR_MARKS_2 GColorShockingPink
#define COLOR_HAND GColorShockingPink
#define COLOR_DATE GColorBlack

static Window *s_window;
static Layer *s_layer_background_ticks, *s_layer_date, *s_hands_layer;
static TextLayer *s_text_layer_weekday, *s_text_layer_day, *s_text_layer_step;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[4], s_day_buffer[6];
static BitmapLayer *s_bitmap_layer_background;
static GBitmap *s_background_bitmap = 0;

static GBitmap *s_bitmap_hour;
static GBitmap *s_bitmap_minute;
static RotBitmapLayer  *s_rotate_layer_hour;
static RotBitmapLayer  *s_rotate_layer_minute;

static void choose_background_bitmap( void ) {
	static GBitmap *new_background_bitmap;
	time_t now = time(NULL);
	struct tm *time = localtime(&now);
	if ( time->tm_wday == 0 || time->tm_wday == 6 )	//sunday and saturday
	{
		//if( time->tm_hour <= 8 )
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_HolidaySleep );
		//else
		//	new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_HOLIDAY );
	}
	else
	{
		switch ( time->tm_hour )
		{
		case 6:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_0600 );
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
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1200 );
			break;
		case 13:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1300 );
			break;
		case 15:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1500 );
			break;
		case 17:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1700 );
			break;
		case 18:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1800 );
			break;
		case 19:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_1900 );
			break;
		case 20:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_2000 );
			break;
		case 22:
			new_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_2200 );
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
	bitmap_layer_set_bitmap(s_bitmap_layer_background, new_background_bitmap);
	if ( 0 != s_background_bitmap )
	gbitmap_destroy(s_background_bitmap);
	s_background_bitmap = new_background_bitmap;
}

void set_hand_angle(RotBitmapLayer *hand_image_container, unsigned int hand_angle) {

  signed short x_fudge = 0;
  signed short y_fudge = 0;


  rot_bitmap_layer_set_angle(hand_image_container, TRIG_MAX_ANGLE * hand_angle / 360);

  //
  // Due to rounding/centre of rotation point/other issues of fitting
  // square pixels into round holes by the time hands get to 6 and 9
  // o'clock there's off-by-one pixel errors.
  //
  // The `x_fudge` and `y_fudge` values enable us to ensure the hands
  // look centred on the minute marks at those points. (This could
  // probably be improved for intermediate marks also but they're not
  // as noticable.)
  //
  // I think ideally we'd only ever calculate the rotation between
  // 0-90 degrees and then rotate again by 90 or 180 degrees to
  // eliminate the error.
  //
  if (hand_angle == 180) {
    x_fudge = -1;
  } else if (hand_angle == 270) {
    y_fudge = -1;
  }

  // (144 = screen width, 168 = screen height)
  GRect frame = layer_get_frame(bitmap_layer_get_layer((BitmapLayer *)hand_image_container));
  frame.origin.x = (144/2) - (frame.size.w/2) + x_fudge;
  frame.origin.y = (168/2) - (frame.size.h/2) + y_fudge;

  layer_set_frame(bitmap_layer_get_layer((BitmapLayer *)hand_image_container), frame);
  layer_mark_dirty(bitmap_layer_get_layer((BitmapLayer *)hand_image_container));
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
	//graphics_context_set_fill_color(ctx, GColorBlack);
	//graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
	//graphics_context_set_fill_color(ctx, COLOR_MARKS);
	graphics_context_set_stroke_color(ctx, GColorBlack );
	for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
		if (  0 == i
				|| 1 == i
				|| 2 == i
				|| 8 == i
				|| 9 == i    )
		{graphics_context_set_fill_color(ctx, COLOR_MARKS_1);}
		else
		{graphics_context_set_fill_color(ctx, COLOR_MARKS_2);}
		const int x_offset = PBL_IF_ROUND_ELSE(18, 0);
		const int y_offset = PBL_IF_ROUND_ELSE(6, 0);
		gpath_move_to(s_tick_paths[i], GPoint(x_offset, y_offset));
		gpath_draw_filled(ctx, s_tick_paths[i]);
		//gpath_draw_outline(ctx, s_tick_paths[i]);
	}
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	//GPoint center = grect_center_point(&bounds);

	//const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	/*	
	int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
	GPoint second_hand = {
		.x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
		.y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
	};

	// second hand
	graphics_context_set_stroke_color(ctx, COLOR_HAND);
	graphics_draw_line(ctx, second_hand, center);
	*/
	// minute/hour hand
	graphics_context_set_fill_color(ctx, COLOR_HAND);
	graphics_context_set_stroke_color(ctx, GColorClear);

	gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
	gpath_draw_filled(ctx, s_minute_arrow);
	gpath_draw_outline(ctx, s_minute_arrow);

	gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
	gpath_draw_filled(ctx, s_hour_arrow);
	gpath_draw_outline(ctx, s_hour_arrow);

	// dot in the middle
	graphics_context_set_fill_color(ctx, GColorShockingPink);
	//graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 13, 13), 0, GCornerNone);
	graphics_fill_circle(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), 2 );
	
	static int old_hour = -1;
	if ( old_hour != t->tm_hour )
	{
		choose_background_bitmap();
		old_hour = t->tm_hour;
	}
}

static void date_update_proc(Layer *layer, GContext *ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
	text_layer_set_text(s_text_layer_weekday, s_day_buffer);

	strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
	text_layer_set_text(s_text_layer_day, s_num_buffer);
}

void update_hand_positions(struct tm *t) {

  set_hand_angle( s_rotate_layer_hour, ((t->tm_hour % 12) * 30) + (t->tm_min/2) );
  set_hand_angle( s_rotate_layer_minute, t->tm_min * 6 );

}

static void tick_service_callback(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(window_get_root_layer(s_window));
	if ( (units_changed & DAY_UNIT) != 0 )
	{
		layer_mark_dirty( s_layer_date );
	}
	if ( (units_changed & HOUR_UNIT) != 0 )
	{
		choose_background_bitmap();
	}
	if ( (units_changed & MINUTE_UNIT) != 0 )
	{
		update_hand_positions( tick_time );
		if ( is_health_updated() )
		{
	  	health_reload_averages(0);
			text_layer_set_text( s_text_layer_step, health_get_current_steps_buffer() );
		}
	}
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	health_init();
	
	// Create background image
	s_bitmap_layer_background = bitmap_layer_create( GRect(16, 5, 112, 122) );
	bitmap_layer_set_background_color( s_bitmap_layer_background, GColorClear );
	bitmap_layer_set_compositing_mode( s_bitmap_layer_background, GCompOpSet );
	choose_background_bitmap();
	layer_add_child( window_layer, bitmap_layer_get_layer(s_bitmap_layer_background));
	
	// Create background ticks
	s_layer_background_ticks = layer_create(bounds);
	layer_set_update_proc(s_layer_background_ticks, bg_update_proc);
	layer_add_child(window_layer, s_layer_background_ticks);
	
	// Create hand layers
	/*
	s_hands_layer = layer_create(bounds);
	layer_set_update_proc(s_hands_layer, hands_update_proc);
	layer_add_child(window_layer, s_hands_layer);
	*/
	//Create bitmap
	s_bitmap_hour = gbitmap_create_with_resource( RESOURCE_ID_HAND_HOUR );
	s_bitmap_minute = gbitmap_create_with_resource( RESOURCE_ID_HAND_MINUTE );
	
	//Create s_rotate_layer_hour
	s_rotate_layer_hour = rot_bitmap_layer_create( s_bitmap_hour );
	rot_bitmap_set_compositing_mode( s_rotate_layer_hour, GCompOpSet);
	rot_bitmap_set_src_ic( s_rotate_layer_hour, GPoint(11, 45));
	
	//Create s_rotate_layer_minute
	s_rotate_layer_minute = rot_bitmap_layer_create( s_bitmap_minute );
	rot_bitmap_set_compositing_mode( s_rotate_layer_minute, GCompOpSet);
	rot_bitmap_set_src_ic( s_rotate_layer_minute, GPoint(6, 63));

	//init hand position
	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = localtime(&rawtime);
	update_hand_positions(timeinfo);
	
	//Add to layer
	layer_add_child( window_layer, bitmap_layer_get_layer( (BitmapLayer*)s_rotate_layer_hour ) );
	layer_add_child( window_layer, bitmap_layer_get_layer( (BitmapLayer*)s_rotate_layer_minute ) );
	
	// Create date layers
	s_layer_date = layer_create(bounds);
	layer_set_update_proc(s_layer_date, date_update_proc);
	layer_add_child(window_layer, s_layer_date);

	s_text_layer_weekday = text_layer_create(PBL_IF_ROUND_ELSE(
	GRect(63, 114, 27, 20),
	GRect(46, 114, 37, 30)));
	text_layer_set_text(s_text_layer_weekday, s_day_buffer);
	text_layer_set_background_color(s_text_layer_weekday, GColorClear);
	text_layer_set_text_color(s_text_layer_weekday, COLOR_DATE );
	text_layer_set_font(s_text_layer_weekday, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(s_layer_date, text_layer_get_layer(s_text_layer_weekday));

	s_text_layer_day = text_layer_create(PBL_IF_ROUND_ELSE(
	GRect(90, 114, 18, 20),
	GRect(78, 114, 37, 30)));
	text_layer_set_text(s_text_layer_day, s_num_buffer);
	text_layer_set_background_color(s_text_layer_day, GColorClear);
	text_layer_set_text_color(s_text_layer_day, COLOR_DATE);
	text_layer_set_font(s_text_layer_day, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(s_layer_date, text_layer_get_layer(s_text_layer_day));
	
	//step layer
	s_text_layer_step = text_layer_create( GRect(40, 134, 64, 24) );
	text_layer_set_text( s_text_layer_step, health_get_current_steps_buffer() );
	text_layer_set_text_alignment( s_text_layer_step, GTextAlignmentCenter );
	//static char test[] = "\U0001F4951,234";
	//text_layer_set_text( s_text_layer_step, test );
	text_layer_set_background_color(s_text_layer_step, GColorClear);
	text_layer_set_text_color( s_text_layer_step, GColorBlack );
	text_layer_set_font( s_text_layer_step, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child( window_layer, text_layer_get_layer(s_text_layer_step));
	health_reload_averages(0);
}

static void window_unload(Window *window) {
	
	text_layer_destroy(s_text_layer_step);
	text_layer_destroy(s_text_layer_day);
	text_layer_destroy(s_text_layer_weekday);
	layer_destroy(s_layer_date);
	
	//layer_destroy(s_hands_layer);
	rot_bitmap_layer_destroy(s_rotate_layer_minute);
	rot_bitmap_layer_destroy(s_rotate_layer_hour);
	
	layer_destroy(s_layer_background_ticks);
	
	bitmap_layer_destroy(s_bitmap_layer_background);
	gbitmap_destroy(s_background_bitmap);
	
	health_deinit();
	
}

static void init() {
	s_window = window_create();
	window_set_window_handlers(s_window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_stack_push(s_window, true);

	s_day_buffer[0] = '\0';
	s_num_buffer[0] = '\0';

	/* // init hand paths
	s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
	s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
	*/
	Layer *window_layer = window_get_root_layer(s_window);
	GRect bounds = layer_get_bounds(window_layer);
	GPoint center = grect_center_point(&bounds);

	/*
	gpath_move_to(s_minute_arrow, center);
	gpath_move_to(s_hour_arrow, center);
	*/
	for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
		s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
	}

	tick_timer_service_subscribe(MINUTE_UNIT, tick_service_callback);
}

static void deinit() {
	/* 
	gpath_destroy(s_hour_arrow);
	gpath_destroy(s_minute_arrow);
	*/
	tick_timer_service_unsubscribe();

	for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
		gpath_destroy(s_tick_paths[i]);
	}

	window_destroy(s_window);
}

int main() {
	init();
	app_event_loop();
	deinit();
}
