#include "simple_analog.h"

#include "pebble.h"

#define COLOR_PURPLE GColorFromRGB(137,92,129)
#define COLOR_SILVER GColorFromRGB(167,158,151)

#define COLOR_MARKS_1 GColorShockingPink
#define COLOR_MARKS_2 GColorShockingPink
#define COLOR_HAND GColorShockingPink
#define COLOR_DATE GColorBlack

static Window *s_window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label;

static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[4], s_day_buffer[6];
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap = 0;

static void choose_background_bitmap( void )
{
	GBitmap *new_background_bitmap;
	time_t now = time(NULL);
    struct tm *time = localtime(&now);
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
	bitmap_layer_set_bitmap(s_background_layer, new_background_bitmap);
	if ( 0 != s_background_bitmap )
		gbitmap_destroy(s_background_bitmap);
	s_background_bitmap = new_background_bitmap;
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
  GPoint center = grect_center_point(&bounds);

  const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

  // second hand
  graphics_context_set_stroke_color(ctx, COLOR_HAND);
  graphics_draw_line(ctx, second_hand, center);

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
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
  if ( (units_changed & HOUR_UNIT) != 0 )
  	choose_background_bitmap();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set background image
  //s_background_bitmap = gbitmap_create_with_resource( RESOURCE_ID_main );
  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);
  // Set the bitmap onto the layer and add to the window
  choose_background_bitmap();
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));	
	
  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
	
  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  s_day_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(63, 114, 27, 20),
    GRect(46, 114, 37, 30)));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorClear);
  text_layer_set_text_color(s_day_label, COLOR_DATE );
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  s_num_label = text_layer_create(PBL_IF_ROUND_ELSE(
    GRect(90, 114, 18, 20),
    GRect(78, 114, 37, 30)));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, GColorClear);
  text_layer_set_text_color(s_num_label, COLOR_DATE);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);

  layer_destroy(s_hands_layer);
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);
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

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
