#include <pebble.h>

extern bool s_is_blinking;

extern float s_smile_phase; // 0 = neutral, 1 = full smile
extern bool s_is_smile_animating;

void draw_face_layer(GContext *ctx, GPoint center, struct tm *tick_time);