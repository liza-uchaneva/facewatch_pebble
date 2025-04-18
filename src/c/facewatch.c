#include <math.h>
#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static Layer *s_canvas_layer;

GPoint screencenter;
#define MINUTE_HAND_LENGTH 55
#define HOUR_HAND_LENGTH 45

#define COLOR_CORAL GColorFromHEX(0xFF3B1F)
#define COLOR_YELLOW GColorFromHEX(0xFFA500)
#define COLOR_FROSTED_BLUE GColorFromHEX(0x7CAAC8)
#define COLOR_DARK_GRAY GColorFromHEX(0x2C2E33)

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

  //Draw shadow
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_context_set_stroke_width(ctx, 9);
  graphics_draw_line(ctx, start, end);

  //Draw a inner line
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_line(ctx, start, end);
}

static void draw_nose(Layer *layer, GContext *ctx, int size)
{
  GPoint shadow_blue_center = GPoint(screencenter.x + 1, screencenter.y + 3);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, COLOR_FROSTED_BLUE);
  graphics_draw_circle(ctx, shadow_blue_center, size);

  GPoint shadow_red_center = GPoint(screencenter.x + 2, screencenter.y - 2);
  graphics_context_set_stroke_color(ctx, COLOR_CORAL);
  graphics_draw_circle(ctx, shadow_red_center, size);

  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_circle(ctx, screencenter, size);
}

static void draw_eye(Layer *layer, GContext *ctx, int side)
{
  // Calculate base x depending on side (-1 for left, 1 for right)
  int base_x = screencenter.x + side * (screencenter.x / 2 - 1);
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
  GPoint start = GPoint(base_x + 10 * side, base_y);
  GPoint end   = GPoint(base_x + 19 * side, base_y);
  graphics_draw_line(ctx, start, end);
}

static void draw_cheek(Layer *layer, GContext *ctx, int side)
{
  // Calculate cheek position based on side (-1 for left, 1 for right)
  int offset_x = side * (screencenter.x / 2 + (side == -1 ? 18 : -14));
  GPoint point = GPoint(screencenter.x + offset_x, screencenter.y - 10);

  GRect grect_cheek = GRect(point.x, point.y, 35, 25);
  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_rect(ctx, grect_cheek, 20, GCornersAll);
}

static void draw_brow(Layer *layer, GContext *ctx, int side)
{
  // side: -1 for left, 1 for right
  int base_x = screencenter.x + side * (screencenter.x / 2 + (side == -1 ? 0 : -4));
  int base_y = screencenter.y - screencenter.y / 4 + 1;

  GRect brow_rect = GRect(base_x - 20, base_y - 35, 45, 45);

  graphics_context_set_stroke_width(ctx, 10);
  graphics_context_set_stroke_color(ctx, COLOR_YELLOW);

  // Mirror angles: right brow uses reversed arc to look symmetrical
  if (side == -1) {
    graphics_draw_arc(ctx, brow_rect, GOvalScaleModeFitCircle,
                      DEG_TO_TRIGANGLE(40), DEG_TO_TRIGANGLE(60));
  } else {
    graphics_draw_arc(ctx, brow_rect, GOvalScaleModeFitCircle,
                      DEG_TO_TRIGANGLE(-60), DEG_TO_TRIGANGLE(-40));
  }
}

static void draw_mouth(Layer *layer, GContext *ctx)
{
  GRect grect_mouth = GRect(screencenter.x - 24, screencenter.y - 15, 50, 50);
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_arc(ctx, grect_mouth, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-230), DEG_TO_TRIGANGLE(-130));

  GRect grect_mouth_shadow = GRect(screencenter.x - 30, screencenter.y - 18, 65, 65);
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_arc(ctx, grect_mouth_shadow, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-185), DEG_TO_TRIGANGLE(-170));

}

static void canvas_update_proc(struct Layer *layer, GContext *ctx)
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // //Draw eyes
  draw_eye(layer, ctx, -1);
  draw_eye(layer, ctx, 1);

  //Draw cheeks
  draw_cheek(layer, ctx, -1);
  draw_cheek(layer, ctx, 1);

  //Draw brows
  draw_brow(layer, ctx, -1);
  draw_brow(layer, ctx, 1);

  //Draw mouth
  draw_mouth(layer, ctx);

  //Draw min hand
  int minutes = tick_time->tm_min;
  draw_hand(layer, ctx, MINUTE_HAND_LENGTH, angle(minutes, 60), COLOR_CORAL);

  //Draw hour hand
  const int hour = tick_time->tm_hour % 12;
  float ang = angle(hour * 50 + minutes * 50 / 60, 600);
  draw_hand(layer, ctx, HOUR_HAND_LENGTH, ang, COLOR_YELLOW);

  //Draw nose
  draw_nose(layer, ctx, 10);
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