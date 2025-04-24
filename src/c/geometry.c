#include <math.h>
#include <pebble.h>

float calculate_angle(int value, int max) {
  return (value == 0 || value == max) ? 0 : (TRIG_MAX_ANGLE * value / max);
}

GPoint get_point_on_circle(int angle, int radius, GPoint screencenter) {
  GRect bounds = GRect(screencenter.x - radius + 1, screencenter.y - radius + 1, radius * 2, radius * 2);
  return gpoint_from_polar(bounds, GOvalScaleModeFitCircle, angle);
}