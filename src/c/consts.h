#include <pebble.h>

#define ANIMATION_INTERVAL 50
#define ANIMATION_INTERVAL_LOW_POWER 100  // Slower updates when battery is low
#define LOW_BATTERY_THRESHOLD 20  // Consider battery low at 20%

#define MINUTE_HAND_LENGTH 55
#define HOUR_HAND_LENGTH 45

#define COLOR_CORAL GColorFromHEX(0xFF3B1F)
#define COLOR_YELLOW GColorFromHEX(0xFFA500)
#define COLOR_FROSTED_BLUE GColorFromHEX(0x7CAAC8)
#define COLOR_DARK_GRAY GColorFromHEX(0x2C2E33)