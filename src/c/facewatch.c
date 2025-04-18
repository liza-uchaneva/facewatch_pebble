#include <math.h>
#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static Layer *s_canvas_layer;

GPoint screencenter;
#define MINUTE_HAND_LENGTH 50
#define HOUR_HAND_LENGTH 42

#define COLOR_CORAL GColorFromHEX(0xEE6352) //cheeck, minute hand, outer line in eyeball
#define COLOR_YELLOW GColorFromHEX(0xFFE156) //eye ball, brows, hour hand
#define COLOR_FROSTED_BLUE GColorFromHEX(0xA4BCCC) //shadows and eye backgrownd
#define COLOR_DARK_GRAY GColorFromHEX(0x454851) //nose, mouth eyelids, lushes

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

static void draw_hand(Layer *layer, GContext *ctx, int length, float angle, GColor color)
{
  //Find start point
  const GPoint start = gpoint_on_circle(angle, 20);
  //Find end point
  const GPoint end = gpoint_on_circle(angle, length);

  // //Draw shadow
  //Set color
  graphics_context_set_stroke_color(ctx, color);
  // Set width
  graphics_context_set_stroke_width(ctx, 5);
  //Draw a line
  graphics_draw_line(ctx, start, end);
}

static void draw_nose(Layer *layer, GContext *ctx, int size)
{
  //Set color
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_circle(ctx, screencenter, size);
}

static void draw_eye_side(Layer *layer, GContext *ctx, int side)
{
  // Calculate base x depending on side (-1 for left, 1 for right)
  int base_x = screencenter.x + side * (screencenter.x / 2 - 1);

  // Base Y remains the same
  int base_y = screencenter.y - screencenter.y / 4 + 1;

  // Draw eye background
  GRect eye_rect = GRect(base_x - 20, base_y - 20, 40, 40);
  graphics_context_set_fill_color(ctx, COLOR_FROSTED_BLUE);
  graphics_fill_radial(ctx, eye_rect, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));

  // Eye ball outer
  GRect eyeball_outer = GRect(base_x - 17, base_y - 37, 35, 35);
  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_radial(ctx, eyeball_outer, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-255), DEG_TO_TRIGANGLE(-115));

  // Eye ball inner
  GRect eyeball_inner = GRect(base_x - 13, base_y - 33, 27, 27);
  graphics_context_set_fill_color(ctx, COLOR_YELLOW);
  graphics_fill_radial(ctx, eyeball_inner, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-255), DEG_TO_TRIGANGLE(-115));

  // Upper eyelid
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_arc(ctx, eye_rect, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));

  // Bottom eyelid line
  GPoint start = GPoint(base_x - 10, base_y);
  GPoint end   = GPoint(base_x - 20, base_y);
  graphics_draw_line(ctx, start, end);
}

// static void draw_eye(Layer *layer, GContext *ctx)
// {
//   const GRect grect_first_eye = GRect(screencenter.x - screencenter.x / 2 + 1 - 20,
//                                 screencenter.y - screencenter.y / 4 + 1 - 20,
//                                 40, 40);
//   // Draw left eye background
//   graphics_context_set_fill_color(ctx, COLOR_FROSTED_BLUE);
//   graphics_fill_radial(ctx, grect_first_eye, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));
  
//   // Draw eye ball
//   const GRect grect_eyeball_outer = GRect(screencenter.x - screencenter.x / 2 + 1 - 17,
//                                 screencenter.y - screencenter.y / 4 + 1 - 37,
//                                 35, 35);
//   graphics_context_set_fill_color(ctx, COLOR_CORAL);
//   graphics_fill_radial(ctx, grect_eyeball_outer, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-255), DEG_TO_TRIGANGLE(-115));
  
//   //Inner eyeball
//   const GRect grect_eyeball_inner = GRect(screencenter.x - screencenter.x / 2 + 1 - 13,
//                                 screencenter.y - screencenter.y / 4 + 1 - 33,
//                                 27, 27);
//   graphics_context_set_fill_color(ctx, COLOR_YELLOW);
//   graphics_fill_radial(ctx, grect_eyeball_inner, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-255), DEG_TO_TRIGANGLE(-115));
  
//   //Draw left upper eyelid
//   graphics_context_set_stroke_width(ctx, 6);
//   graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
//   graphics_draw_arc(ctx, grect_first_eye, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));

//   // Draw left bottom eyelid
//   GPoint start = GPoint(screencenter.x - screencenter.x / 2 + 1 -10, 
//                      screencenter.y - screencenter.y / 4 + 1);
//   GPoint end = GPoint(screencenter.x - screencenter.x / 2 + 1 - 20, 
//                      screencenter.y - screencenter.y / 4 + 1);
//   graphics_draw_line(ctx, start, end);
// }

static void canvas_update_proc(struct Layer *layer, GContext *ctx)
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  //Draw eyes
  draw_eye_side(layer, ctx, -1);
  draw_eye_side(layer, ctx, 1);

  //Draw min hand
  int minutes = tick_time->tm_min;
  draw_hand(layer, ctx, MINUTE_HAND_LENGTH, angle(minutes, 60), COLOR_CORAL);

  //Draw hour hand
  const int hour = tick_time->tm_hour % 12;
  float ang = angle(hour * 50 + minutes * 50 / 60, 600);
  draw_hand(layer, ctx, HOUR_HAND_LENGTH, ang, COLOR_YELLOW);

  //Draw nose
  draw_nose(layer, ctx, 8);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  //Find screen center
  screencenter = GPoint(bounds.size.w/2, bounds.size.h/2);

  // Create canvas layer
  s_canvas_layer = layer_create(bounds);
  if(!s_canvas_layer)
  {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create canvas layer");
    return;
  }
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
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