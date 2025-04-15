#include <math.h>
#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static Layer *s_canvas_layer;

GPoint screencenter;
#define MINUTE_HAND_LENGTH 48
#define HOUR_HAND_LENGTH 40

float angle(const int value, const int max){
  if(value == 0 || value == max)
    return 0;
  return TRIG_MAX_ANGLE * value / max;
}

GPoint gpoint_on_circle(const int angle, const int radius){
  const int diameter = radius * 2;
  const GRect grect_for_polar = GRect(screencenter.x - radius + 1, screencenter.y - radius + 1, diameter, diameter);
  return gpoint_from_polar(grect_for_polar, GOvalScaleModeFitCircle, angle);
}

static void draw_hand(Layer *layer, GContext *ctx, int length, float angle)
{
   //Find start point
  const GPoint start = gpoint_on_circle(angle, 15);
  //Find end point
  const GPoint end = gpoint_on_circle(angle, length);

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
  int minutes = tick_time->tm_min;
  draw_hand(layer, ctx, MINUTE_HAND_LENGTH, angle(minutes, 60));

  //Update hour hand
  const int hour = tick_time->tm_hour % 12;
  float ang = angle(hour * 50 + minutes * 50 / 60, 600);
  draw_hand(layer, ctx, HOUR_HAND_LENGTH, ang);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // Create canvas layer
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