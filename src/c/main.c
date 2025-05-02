#include "c/drawing.h"
#include "c/consts.h"

static Window *s_window;
static Layer *s_canvas_layer;
static Layer *s_hands_layer;
static Layer *s_battery_layer;

// Battery state
static int s_battery_level = 100;
bool s_is_charging = false;

// Blink animation
bool s_is_blinking = false;
static AppTimer *s_blink_animation_timer;

// Smile animation
bool s_is_smile_animating = false;
bool is_smile_increacing = true;
float s_smile_phase = 0;
static AppTimer *s_smile_animation_timer;

GPoint screencenter;

static void battery_update_proc(Layer *layer, GContext *ctx) {
  BatteryChargeState charge_state = battery_state_service_peek();

  // Battery style constants
  const int BATTERY_WIDTH = 22;
  const int BATTERY_HEIGHT = 10;
  const int BATTERY_CAP_WIDTH = 2;

  GRect bounds = layer_get_bounds(layer);
  GPoint origin = GPoint((bounds.size.w - BATTERY_WIDTH - BATTERY_CAP_WIDTH) / 2, 
                         (bounds.size.h - BATTERY_HEIGHT) / 2);

  // Fill level
  int percent = charge_state.charge_percent;
  int fill_width = (percent * BATTERY_WIDTH) / 100;
  GRect fill_rect = GRect(origin.x + 1, origin.y + 1, fill_width - 2, BATTERY_HEIGHT - 2);
  GColor fill_color = (percent >= 70) ? COLOR_DARK_GRAY :
                      (percent >= 40) ? COLOR_YELLOW    :
                                        COLOR_CORAL;
  graphics_context_set_fill_color(ctx, fill_color);
  if (fill_width > 2) {  // prevent negative width
    graphics_fill_rect(ctx, fill_rect, 0, GCornerNone);
  }

  // Battery body
  GRect battery_body = GRect(origin.x, origin.y, BATTERY_WIDTH, BATTERY_HEIGHT);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_rect(ctx, battery_body);

  // Battery cap
  graphics_context_set_fill_color(ctx, COLOR_DARK_GRAY);
  GRect battery_cap = GRect(origin.x + BATTERY_WIDTH, origin.y + 2, BATTERY_CAP_WIDTH, BATTERY_HEIGHT - 4);
  graphics_fill_rect(ctx, battery_cap, 0, GCornerNone);
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
  draw_face_layer(ctx, screencenter);
}

static void hands_update_proc(Layer *layer, GContext *ctx)
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  draw_hands_layer(ctx, screencenter, tick_time);
}

static void blink_animation_timer_callback(void *data) {
  if(!s_is_smile_animating && !s_is_blinking)
  {
    s_is_blinking = true;
    psleep(100);
    s_blink_animation_timer = app_timer_register(100, blink_animation_timer_callback, NULL);
  }
  else{
    s_is_blinking = false;
  }
  layer_mark_dirty(s_canvas_layer);
}


static void smiling_animation_timer_callback(void *data) {
  if(s_is_smile_animating)
  {
    if(is_smile_increacing)
    {
      s_smile_phase += 0.05;
      APP_LOG(APP_LOG_LEVEL_DEBUG,"smile_phase is increasing");
      if(s_smile_phase >= 1.0)
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG,"smile_phase on hold");
        psleep(10000);
        is_smile_increacing = false;
      }
        s_smile_animation_timer = app_timer_register(100, smiling_animation_timer_callback, NULL);
    }
    else{
      s_smile_phase -= 0.05;
      APP_LOG(APP_LOG_LEVEL_DEBUG,"smile_phase decreasing");
      if(s_smile_phase <= 0.0)
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG,"smile is ended");
        s_is_smile_animating = false;
        is_smile_increacing = true;
        s_smile_phase = 0.0;
        s_smile_animation_timer = app_timer_register(72000, smiling_animation_timer_callback, NULL);
      }
      else{
      s_smile_animation_timer = app_timer_register(100, smiling_animation_timer_callback, NULL);
      }
    }
  }
  else{
    s_is_smile_animating = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG,"smile is started");
    s_smile_animation_timer = app_timer_register(10, smiling_animation_timer_callback, NULL);
  }

  layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  screencenter = GPoint(bounds.size.w / 2, bounds.size.h / 2);

  // Face layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  //Hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);

  //Battary layer
  GRect battery_frame;
  battery_frame.origin.x = bounds.size.w - 31;
  battery_frame.origin.y = 5;
  battery_frame.size.w = 25;
  battery_frame.size.h = 10;

  s_battery_layer = layer_create(battery_frame);    
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  // s_smile_animation_timer = app_timer_register(720, smiling_animation_timer_callback, NULL);
  layer_mark_dirty(s_canvas_layer);
}

static void prv_window_unload(Window *window) {
  // Cancel the animation timer if it exists
    if (s_smile_animation_timer) {
        app_timer_cancel(s_smile_animation_timer);
        s_smile_animation_timer = NULL;
    }

  layer_destroy(s_canvas_layer);
  layer_destroy(s_hands_layer);

  if (s_battery_layer) {
        layer_destroy(s_battery_layer);
        s_battery_layer = NULL;
  }
}

static void prv_init(void) {
  s_smile_animation_timer = NULL;

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);  // Use callback function
    
  // Get initial battery state
  BatteryChargeState state = battery_state_service_peek();
  s_battery_level = state.charge_percent;
  s_is_charging = state.is_charging;
}

static void prv_deinit(void) {
  // Cancel the animation timer if it exists
  if (s_smile_animation_timer) {
      app_timer_cancel(s_smile_animation_timer);
      s_smile_animation_timer = NULL;
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