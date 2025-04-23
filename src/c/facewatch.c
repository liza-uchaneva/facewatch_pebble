#include <math.h>
#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static Layer *s_canvas_layer;
static Layer *s_battery_layer;

// Battery state
static int s_battery_level = 100;
static bool s_is_charging = false;
static AppTimer *s_animation_timer;

// Eyes animations stats
static bool s_is_blinking = false;
static int s_blink_timer = 0;

static float s_smile_phase = 0;  // 0 = neutral, 1 = full smile
static bool s_is_smile_animating = false;
static int s_smile_timer = 0;

// Update frequency
#define ANIMATION_INTERVAL 50
#define ANIMATION_INTERVAL_LOW_POWER 100  // Slower updates when battery is low
#define LOW_BATTERY_THRESHOLD 20  // Consider battery low at 20%

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

  if(s_is_smile_animating)
  {
    GRect eye_bg = GRect(base_x - 20, base_y - 20, 40, 40);
    graphics_context_set_stroke_width(ctx, 6);
    graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
    graphics_draw_arc(ctx, eye_bg, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(-90), DEG_TO_TRIGANGLE(90));
    return;
  }

  if (s_is_blinking) {
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_line(ctx, GPoint(base_x - 15, base_y), GPoint(base_x + 15, base_y));
  return;
  }
  
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

  int normal_start = (side == -1) ? 40 : -60;
  int normal_end   = (side == -1) ? 60 : -40;

  int smile_start  = (side == -1) ? -30 : 10;
  int smile_end    = (side == -1) ? -10 : 30;

  int start_angle = normal_start + (int)((smile_start - normal_start) * s_smile_phase);
  int end_angle   = normal_end + (int)((smile_end - normal_end) * s_smile_phase);

  GRect brow_rect = GRect(base_x - 20, base_y - 35, 45, 45);
  graphics_context_set_stroke_width(ctx, 10);
  graphics_context_set_stroke_color(ctx, COLOR_YELLOW);
  graphics_draw_arc(ctx, brow_rect, GOvalScaleModeFitCircle,
                    DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(end_angle));
}

static void draw_mouth(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_width(ctx, 6);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);

  float smile = s_smile_phase;

  // Interpolate mouth arc angles
  int start_angle = -230 + (int)(-20 * smile);  // goes to -240
  int end_angle   = -130 - (int)(-20 * smile);  // goes to -120

  graphics_draw_arc(ctx, GRect(screencenter.x - 24, screencenter.y - 15 - (int)(3 * smile), 50, 50),
                    GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(end_angle));

  int top_start = -185;
  int top_end = -170;
  graphics_draw_arc(ctx, GRect(screencenter.x - 30, screencenter.y - 18 - (int)(3 * smile), 65, 65),
                    GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(top_start), DEG_TO_TRIGANGLE(top_end));
}

static void animation_update() {
  // // Blinking logic
  // s_blink_timer++;
  // if (s_blink_timer >= 368) { // blink every ~15 seconds
  //   s_blink_timer = 0;
  //   s_is_blinking = true;
  // } else if (s_blink_timer == 3) {
  //   s_is_blinking = false;
  // }

  s_smile_timer++;
  if (s_smile_timer >= 360) {
    s_smile_timer = 0;
    s_is_smile_animating = true;
  }

  // Smile animation phase control
  if (s_is_smile_animating) {
    s_smile_phase += 0.02f; // Speed of smile
    if (s_smile_phase >= 1.0f) {
      s_smile_phase = 1.0f;
      s_is_smile_animating = false;
    }
  } else {
    if (s_smile_phase > 0) {
      s_smile_phase -= 0.05f;
      if (s_smile_phase < 0) s_smile_phase = 0;
    }
  }

  // Redraw the canvas with updated animation values
  layer_mark_dirty(s_canvas_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  BatteryChargeState charge_state = battery_state_service_peek();

  // Battery style constants
  const int BATTERY_WIDTH = 22;
  const int BATTERY_HEIGHT = 10;
  const int BATTERY_CAP_WIDTH = 2;
  
  GRect bounds = layer_get_bounds(layer);
  GPoint origin = GPoint((bounds.size.w - BATTERY_WIDTH - BATTERY_CAP_WIDTH) / 2, 
                         (bounds.size.h - BATTERY_HEIGHT) / 2);

  // Battery body
  GRect battery_body = GRect(origin.x, origin.y, BATTERY_WIDTH, BATTERY_HEIGHT);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_rect(ctx, battery_body);

  // Battery cap (optional for aesthetics)
  GRect battery_cap = GRect(origin.x + BATTERY_WIDTH, origin.y + 2, BATTERY_CAP_WIDTH, BATTERY_HEIGHT - 4);
  graphics_fill_rect(ctx, battery_cap, 0, GCornerNone);

  // Fill level
  int fill_width = (charge_state.charge_percent * BATTERY_WIDTH) / 100;
  GRect fill_rect = GRect(origin.x, origin.y, fill_width, BATTERY_HEIGHT);
  graphics_context_set_fill_color(ctx, COLOR_DARK_GRAY);
  graphics_fill_rect(ctx, fill_rect, 0, GCornerNone);
}

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

// Animation timer callback
static void animation_timer_callback(void *data) {
    // First update the animation
    animation_update();
    
    // Determine next interval based on battery level
    uint32_t next_interval = (s_battery_level <= LOW_BATTERY_THRESHOLD && !s_is_charging) ? 
                             ANIMATION_INTERVAL_LOW_POWER : ANIMATION_INTERVAL;
    
    // Simply register the next timer - no complex retry logic needed
    s_animation_timer = app_timer_register(next_interval, animation_timer_callback, NULL);
    
    // If for some reason the timer couldn't be registered, try one more time with a fallback interval
    if (!s_animation_timer) {
        APP_LOG(APP_LOG_LEVEL_WARNING, "Failed to register animation timer, retrying");
        s_animation_timer = app_timer_register(ANIMATION_INTERVAL * 2, animation_timer_callback, NULL);
    }
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

  GRect battery_frame;
  battery_frame.origin.x = bounds.size.w - 25;
  battery_frame.origin.y = 5;
  battery_frame.size.w = 22;
  battery_frame.size.h = 10;
  s_battery_layer = layer_create(battery_frame);
  if (!s_battery_layer) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create battery layer");
      return;
  }
    
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  layer_mark_dirty(s_canvas_layer);

  // Start animation timer with error checking
    s_animation_timer = app_timer_register(ANIMATION_INTERVAL, animation_timer_callback, NULL);
    if (!s_animation_timer) {
        // If timer creation fails, try again with longer interval
        s_animation_timer = app_timer_register(ANIMATION_INTERVAL * 2, animation_timer_callback, NULL);
    }
}

static void prv_window_unload(Window *window) {
  // Cancel the animation timer if it exists
    if (s_animation_timer) {
        app_timer_cancel(s_animation_timer);
        s_animation_timer = NULL;
    }

  layer_destroy(s_canvas_layer);
  gbitmap_destroy(s_bitmap);
  if (s_battery_layer) {
        layer_destroy(s_battery_layer);
        s_battery_layer = NULL;
  }
}

static void prv_init(void) {
  s_animation_timer = NULL;

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
  // Cancel the animation timer if it exists
    if (s_animation_timer) {
        app_timer_cancel(s_animation_timer);
        s_animation_timer = NULL;
    }

  window_destroy(s_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}