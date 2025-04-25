#include "c/drawing.h"
#include "c/consts.h"

static Window *s_window;
static Layer *s_canvas_layer;
static Layer *s_battery_layer;

// Battery state
static int s_battery_level = 100;
bool s_is_charging = false;

static AppTimer *s_animation_timer;

// Blink animation
static int s_blink_timer = 0;
bool s_is_blinking = false;

// Smile animation
bool s_is_smile_animating = false;
float s_smile_phase = 0;
static int s_smile_timer = 0;
static int s_smile_hold_timer = 0;

GPoint screencenter;

static void animation_update() {
  s_blink_timer++;
  if (s_blink_timer >= 728 && !s_is_smile_animating) { // blink every ~15 seconds
    s_blink_timer = 0;
    s_is_blinking = true;
  } else if (s_blink_timer == 3) {
    s_is_blinking = false;
  }
  // Smile logic
  s_smile_timer++;
  if (s_smile_timer >= 7200) {
    s_smile_timer = 0;
    s_is_smile_animating = true;
  }

  // Smile animation phase control
  if (s_is_smile_animating) {
    if(s_smile_phase < 1.0f)
    {
      s_smile_phase += 0.05f;
      if(s_smile_phase == 1.0f) 
      {
        s_smile_phase = 1.0f;
        s_is_smile_animating = false;
      }
    }
    else{
       s_smile_hold_timer++;
      if (s_smile_hold_timer >= 100) {
        s_is_smile_animating = false; // Start un-smiling
      }
    }
  } else {
    if (s_smile_phase > 0) {
      s_smile_phase -= 0.05f;
      if (s_smile_phase < 0) s_smile_phase = 0;
    }
  }
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
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  draw_face_layer(ctx, screencenter, tick_time);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

static void animation_timer_callback(void *data) {
    animation_update();
    

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
  battery_frame.origin.x = bounds.size.w - 31;
  battery_frame.origin.y = 5;
  battery_frame.size.w = 25;
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
  BatteryChargeState state = battery_state_service_peek();
  s_battery_level = state.charge_percent;
  s_is_charging = state.is_charging;
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