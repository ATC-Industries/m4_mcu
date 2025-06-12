#ifndef TOUCH_TOUCHSCREEN_H_
#define TOUCH_TOUCHSCREEN_H_

#include <Arduino.h>

namespace touch {

// Initialize the touch system. Call this in setup().
void init_touch();

// Returns true if the screen is currently being touched.
bool isTouching();

// Reads raw touch coordinates. Returns true if valid.
bool readRaw(uint16_t &x, uint16_t &y, uint16_t &z);

}  // namespace touch

#endif  // TOUCH_TOUCHSCREEN_H_
