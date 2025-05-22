#include <pebble.h>
#include "c/geometry.h"
#include "c/consts.h"
#include "c/health.h"

extern bool s_is_blinking;

float s_smile_phase; // 0 = neutral, 1 = full smile
bool s_is_smile_animating;

void draw_face_layer(GContext *ctx, GPoint center);
void update_face_layer(int steps, int average_steps);
void draw_hands_layer(GContext *ctx, GPoint center, struct tm *tick_time);