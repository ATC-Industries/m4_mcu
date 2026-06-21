// include/touch/touch.h
#ifndef INCLUDE_TOUCH_H_
#define INCLUDE_TOUCH_H_

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_Touch.h>  // Bodmer's library (raw axis reads only)

#include "Config.h"

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

extern bool recalibrateTouch;

// 3x3 grid calibration: 9 nodes, each storing the raw (x,y) measured at a known
// screen location. Runtime uses inverse-bilinear interpolation within the grid
// cell containing the touch. This handles resistive-panel nonlinearity and
// axis cross-coupling that a single affine transform cannot.
struct TouchCalibration {
  int rawX[9];   // measured raw X at each of the 9 grid nodes (row-major)
  int rawY[9];   // measured raw Y at each node
  bool valid;
};

// Call once in setup(), AFTER init_display() and BEFORE init_lvgl().
void init_touch();

// Interactive 9-point grid calibration. Saves on success.
bool calibrate_touch();

bool load_touch_calibration();
bool save_touch_calibration(const TouchCalibration &cal);
bool setRecalibrationFlag(bool force = true);

// LVGL read path: true with mapped screen coords when pressed.
bool readTouchMapped(uint16_t *x, uint16_t *y);

// Accuracy test: tap 9 targets, logs intended-vs-actual error.
void touch_accuracy_test();

// Clean raw read (discard first conversion per axis, median of samples).
bool readTouchRaw(int *rx, int *ry, int samples);

#endif  // INCLUDE_TOUCH_H_