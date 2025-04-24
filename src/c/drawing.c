#include "c/geometry.h"
#include "c/consts.h"

#include <pebble.h>

GPoint screencenter;

static void draw_hand(GContext *ctx, int length, float angle, GColor color) {
  GPoint start = get_point_on_circle(angle, 20, screencenter);
  GPoint end = get_point_on_circle(angle, length, screencenter);

  graphics_context_set_stroke_width(ctx, 9);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_line(ctx, start, end);

  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx, start, end);
}

static void draw_nose(GContext *ctx, int size) {
  graphics_context_set_stroke_width(ctx, 5);

  graphics_context_set_stroke_color(ctx, COLOR_FROSTED_BLUE);
  graphics_draw_circle(ctx, GPoint(screencenter.x + 1, screencenter.y + 3), size);

  graphics_context_set_stroke_color(ctx, COLOR_CORAL);
  graphics_draw_circle(ctx, GPoint(screencenter.x + 2, screencenter.y - 2), size);

  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);
  graphics_draw_circle(ctx, screencenter, size);
}

static void draw_eye(GContext *ctx, int side) {
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

static void draw_cheek(GContext *ctx, int side) {
  int offset_x = side * (screencenter.x / 2 + (side == -1 ? 18 : -14));
  GPoint point = GPoint(screencenter.x + offset_x, screencenter.y - 5 - 3 * s_smile_phase);

  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_rect(ctx, GRect(point.x, point.y, 35, 20), 20, GCornersAll);
}

static void draw_brow(GContext *ctx, int side) {
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

static void draw_mouth(GContext *ctx) {
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

static void draw_lush(GContext *ctx, int side) {
  int base_x = screencenter.x + side * (screencenter.x / 2 - 1);
  int base_y = screencenter.y - screencenter.y / 4 + 1;

  graphics_context_set_stroke_width(ctx, 4);
  graphics_context_set_stroke_color(ctx, COLOR_DARK_GRAY);

  // Neutral and smile endpoints for left/right
  int neutral_offsets[2][4][2] = {
    { {11, 7}, {13, 12}, {18, 7}, {20, 11} },   // Right
    {{11, 7}, {13, 12}, {18, 7}, {20, 11} }     // Left (same shape, but inverted by side)
  };
  int smile_offsets[2][4][2] = {
    { {8, 7}, {-1, 20}, {-3, 7}, {-13, 20} },   // Right smile
    { {2, 7}, {-7, 20}, {-9, 7}, {-19, 20} }    // Left smile
  };

  int s = (side == -1) ? 1 : 0;

  for (int i = 0; i < 2; i++) {
    int x0 = base_x + (int)((1.0f - s_smile_phase) * (neutral_offsets[s][i * 2][0] * side) + s_smile_phase * smile_offsets[s][i * 2][0]);
    int y0 = base_y + (int)((1.0f - s_smile_phase) * neutral_offsets[s][i * 2][1] + s_smile_phase * smile_offsets[s][i * 2][1]);

    int x1 = base_x + (int)((1.0f - s_smile_phase) * (neutral_offsets[s][i * 2 + 1][0] * side) + s_smile_phase * smile_offsets[s][i * 2 + 1][0]);
    int y1 = base_y + (int)((1.0f - s_smile_phase) * neutral_offsets[s][i * 2 + 1][1] + s_smile_phase * smile_offsets[s][i * 2 + 1][1]);

    graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x1, y1));
  }
}

void draw_face_layer(GContext *ctx, GPoint center, struct tm *tick_time){
  screencenter = center;

  draw_eye(ctx, -1);
  draw_eye(ctx, 1);

  draw_cheek(ctx, -1);
  draw_cheek(ctx, 1);

  draw_lush(ctx, -1);
  draw_lush(ctx, 1);

  draw_brow(ctx, -1);
  draw_brow(ctx, 1);

  draw_mouth(ctx);
  draw_nose(ctx, 10);

  draw_hand(ctx, MINUTE_HAND_LENGTH, calculate_angle(tick_time->tm_min, 60), COLOR_CORAL);
  draw_hand(ctx, HOUR_HAND_LENGTH, calculate_angle((tick_time->tm_hour % 12) * 50 + tick_time->tm_min * 50 / 60, 600), COLOR_YELLOW);
}