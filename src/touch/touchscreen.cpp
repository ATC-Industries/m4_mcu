#include "touch/touchscreen.h"

#include <SPI.h>
#include <touch/XPT2046_Touchscreen.h>

#include "Config.h"

namespace {

// Use constants from Config.h
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

bool touch_initialized = false;

}  // namespace

namespace touch {

void init_touch() {
  ts.begin();  // SPI config defaults to VSPI (pins set in hardware)
  ts.setRotation(SCREEN_ROTATION);
  touch_initialized = true;

  Serial.println("Touchscreen initialized.");
}

bool isTouching() {
  if (!touch_initialized) return false;
  return ts.touched();
}

bool readRaw(uint16_t &x, uint16_t &y, uint16_t &z) {
  if (!touch_initialized || !ts.touched()) return false;

  TS_Point p = ts.getPoint();
  x = p.x;
  y = p.y;
  z = p.z;
  return true;
}

}  // namespace touch
