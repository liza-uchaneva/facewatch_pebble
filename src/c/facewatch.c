#include <math.h>
#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static Layer *s_canvas_layer;
static Layer *s_battery_layer;

// Battery state
static int s_battery_level = 100;
static bool s_is_charging = false;

GPoint screencenter;
#define MINUTE_HAND_LENGTH 55
#define HOUR_HAND_LENGTH 45

#define COLOR_CORAL GColorFromHEX(0xFF3B1F)
#define COLOR_YELLOW GColorFromHEX(0xFFA500)
#define COLOR_FROSTED_BLUE GColorFromHEX(0x7CAAC8)
#define COLOR_DARK_GRAY GColorFromHEX(0x2C2E33)

float calculate_angle(int value, int max) {
  return (value == 0 || value == max) ? 0 : (TRIG_MAX_ANGLE * value / max);
}

GPoint get_point_on_circle(int angle, int radius) {
  GRect bounds = GRect(screencenter.x - radius + 1, screencenter.y - radius + 1, radius * 2, radius * 2);
  return gpoint_from_polar(bounds, GOvalScaleModeFitCircle, angle);
}

static void draw_hand(Layer *layer, GContext *ctx, int length, float angle, GColor color) {
  GPoint start = get_point_on_circle(angle, 20);
  GPoint end = get_point_on_circle(angle, length);

  graphics_context_set_stroke_width(ctx, 9);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_line(ctx, start, end);

  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx, start, end);
}

static void draw_nose(Layer *layer, GContext *ctx, int size) {
  graphics_context_set_stroke_width(ctx, 5);

  graphics_context_set_stroke_color(ctx, COLOR_FROSTED_BLUE);
  graphics_draw_circle(ctx, GPoint(screencenter.x + 1, screencenter.y + 3), size);

  graphics_context_set_stroke_color(ctx, COLOR_CORAL);
  graphics_draw_circle(ctx, GPoint(screencenter.x + 2, screencenter.y - 2), size);

  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_circle(ctx, screencenter, size);
}

static void draw_eye(Layer *layer, GContext *ctx, int side) {
  int base_x = screencenter.x + side * (screencenter.x / 2 - 1);
  int base_y = screencenter.y - screencenter.y / 4 + 1;

  GRect eye_bg = GRect(base_x - 20, base_y - 20, 40, 40);
  graphics_context_set_fill_color(ctx, COLOR_FROSTED_BLUE);
  graphics_fill_radial(ctx, eye_bg, GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));

  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_radial(ctx, GRect(base_x - 17, base_y - 37, 35, 35), GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-255), DEG_TO_TRIGANGLE(-115));

  graphics_context_set_fill_color(ctx, COLOR_YELLOW);
  graphics_fill_radial(ctx, GRect(base_x - 13, base_y - 33, 27, 27), GOvalScaleModeFitCircle, 30, DEG_TO_TRIGANGLE(-255), DEG_TO_TRIGANGLE(-115));

  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_arc(ctx, eye_bg, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));

  graphics_draw_line(ctx, GPoint(base_x + 10 * side, base_y), GPoint(base_x + 19 * side, base_y));
}

static void draw_cheek(Layer *layer, GContext *ctx, int side) {
  int offset_x = side * (screencenter.x / 2 + (side == -1 ? 18 : -14));
  GPoint point = GPoint(screencenter.x + offset_x, screencenter.y - 8);

  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_rect(ctx, GRect(point.x, point.y, 35, 20), 20, GCornersAll);
}

static void draw_brow(Layer *layer, GContext *ctx, int side) {
  int base_x = screencenter.x + side * (screencenter.x / 2 + (side == -1 ? 0 : -4));
  int base_y = screencenter.y - screencenter.y / 4 + 1;
  GRect brow_rect = GRect(base_x - 20, base_y - 35, 45, 45);

  graphics_context_set_stroke_width(ctx, 10);
  graphics_context_set_stroke_color(ctx, COLOR_YELLOW);

  graphics_draw_arc(ctx, brow_rect, GOvalScaleModeFitCircle,
    side == -1 ? DEG_TO_TRIGANGLE(40) : DEG_TO_TRIGANGLE(-60),
    side == -1 ? DEG_TO_TRIGANGLE(60) : DEG_TO_TRIGANGLE(-40));
}

static void draw_mouth(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);

  graphics_draw_arc(ctx, GRect(screencenter.x - 24, screencenter.y - 15, 50, 50),
                    GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-230), DEG_TO_TRIGANGLE(-130));

  graphics_draw_arc(ctx, GRect(screencenter.x - 30, screencenter.y - 18, 65, 65),
                    GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-185), DEG_TO_TRIGANGLE(-170));
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  BatteryChargeState charge_state = battery_state_service_peek();

  // Battery style constants
  const int BATTERY_WIDTH = 20;
  const int BATTERY_HEIGHT = 8;
  const int BATTERY_CAP_WIDTH = 2;
  
  GRect bounds = layer_get_bounds(layer);
  GPoint origin = GPoint((bounds.size.w - BATTERY_WIDTH - BATTERY_CAP_WIDTH) / 2, 
                         (bounds.size.h - BATTERY_HEIGHT) / 2);

  // Battery body
  GRect battery_body = GRect(origin.x, origin.y, BATTERY_WIDTH, BATTERY_HEIGHT);
  graphics_context_set_stroke_color(ctx, COLOR_CORAL);
  graphics_draw_rect(ctx, battery_body);

  // Battery cap (optional for aesthetics)
  GRect battery_cap = GRect(origin.x + BATTERY_WIDTH, origin.y + 2, BATTERY_CAP_WIDTH, BATTERY_HEIGHT - 4);
  graphics_fill_rect(ctx, battery_cap, 0, GCornerNone);

  // Fill level
  int fill_width = (charge_state.charge_percent * BATTERY_WIDTH) / 100;
  GRect fill_rect = GRect(origin.x, origin.y, fill_width, BATTERY_HEIGHT);
  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_rect(ctx, fill_rect, 0, GCornerNone);
}

// Battery state handler
static void battery_callback(BatteryChargeState charge_state) {
    s_battery_level = charge_state.charge_percent;
    s_is_charging = charge_state.is_charging;
    
    // Request redraw of battery indicator
    if (s_battery_layer) {
        layer_mark_dirty(s_battery_layer);
    }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  draw_eye(layer, ctx, -1);
  draw_eye(layer, ctx, 1);

  draw_cheek(layer, ctx, -1);
  draw_cheek(layer, ctx, 1);

  draw_brow(layer, ctx, -1);
  draw_brow(layer, ctx, 1);

  draw_mouth(layer, ctx);
  draw_nose(layer, ctx, 10);

  draw_hand(layer, ctx, MINUTE_HAND_LENGTH, calculate_angle(tick_time->tm_min, 60), COLOR_CORAL);
  draw_hand(layer, ctx, HOUR_HAND_LENGTH, calculate_angle((tick_time->tm_hour % 12) * 50 + tick_time->tm_min * 50 / 60, 600), COLOR_YELLOW);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  screencenter = GPoint(bounds.size.w / 2, bounds.size.h / 2);

  s_canvas_layer = layer_create(bounds);
  if (!s_canvas_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create canvas layer");
    return;
  }

  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

// Create battery layer
  GRect battery_frame;
  battery_frame.origin.x = bounds.size.w - 25;
  battery_frame.origin.y = 5;
  battery_frame.size.w = 20;
  battery_frame.size.h = 8;
  s_battery_layer = layer_create(battery_frame);
  if (!s_battery_layer) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create battery layer");
      return;
  }
    
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  gbitmap_destroy(s_bitmap);
  if (s_battery_layer) {
        layer_destroy(s_battery_layer);
        s_battery_layer = NULL;
  }
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);  // Use callback function
    
    // Get initial battery state
  s_battery_level = battery_state_service_peek().charge_percent;
  s_is_charging = battery_state_service_peek().is_charging;
}

static void prv_deinit(void) {
  window_destroy(s_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}