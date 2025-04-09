#include <math.h>
#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static Layer *s_canvas_layer;

GPoint screencenter;
#define M_PI 3.14159265358979323846264338327950288
#define MINUTE_HAND_LENGTH 50
#define HOUR_HAND_LENGTH 46

static double fractionToRadiant(double fraction)
{
  return fraction * 2 * M_PI;
}

static void update_hand(GContext *ctx, const double angle, int length)
{
  //Find start point
  int x1 =  screencenter.x + sin(angle) * 15;
  int y1 = screencenter.y - cos(angle) * 15;
  GPoint start = GPoint(x1,y1);

  //Find end point
  int x2 =  screencenter.x + sin(angle) * length;
  int y2 = screencenter.y - cos(angle) * length;
  GPoint end = GPoint(x2,y2);

  //Set color
  graphics_context_set_stroke_color(ctx, GColorRed);
  // Set width
  graphics_context_set_stroke_width(ctx, 5);
  //Draw a line
  graphics_draw_line(ctx, start, end);
}

static void update_time(struct Layer *layer, GContext *ctx)
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  //Update min hand
  int minute = tick_time->tm_min;
  double minuteFraction =  (double)minute / 60;
  double minuteAngle = fractionToRadiant(minuteFraction);
  update_hand(ctx, minuteAngle, MINUTE_HAND_LENGTH);

  //Update hour hand
  int hour = tick_time->tm_hour; 
  double hourFraction = (hour % 12 + minuteFraction) / 12;
  double hourAngle = fractionToRadiant(hourFraction);
  update_hand(ctx, hourAngle, HOUR_HAND_LENGTH);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  screencenter = GPoint(bounds.size.w/2, bounds.size.h/2);

  layer_set_update_proc(s_canvas_layer, update_time);
  layer_add_child(window_get_root_layer(s_window), s_canvas_layer);

  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_unload(Window *window) {
   layer_destroy(s_canvas_layer);
   gbitmap_destroy(s_bitmap);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}