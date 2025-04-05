// calibration.h
#ifndef INCLUDE_TOUCH_CALIBRATION_H_
#define INCLUDE_TOUCH_CALIBRATION_H_

#include <Arduino.h>

#include "display/display.h"
#include "touch/touch.h"

class TouchCalibration {
public:
  // Run full calibration procedure
  static bool runCalibration(LGFX& lcd, TouchScreen& touch);

private:
  static const int kTargetSize = 20;
  static const int kTextHeight = 40;
  static const int kDebounceDelay = 250;

  // Helper methods
  static void drawTarget(LGFX& lcd, uint16_t x, uint16_t y);
  static void showCalibrationPoint(LGFX& lcd, uint16_t x, uint16_t y, const char* message);
  static void showTouchTest(LGFX& lcd, TouchScreen& touch);
  static void clearTextArea(LGFX& lcd);
  static bool waitForTouch(TouchScreen& touch, uint16_t* x, uint16_t* y, uint16_t* z);
  static bool waitForRelease(TouchScreen& touch);
};

#endif  // INCLUDE_TOUCH_CALIBRATION_H_