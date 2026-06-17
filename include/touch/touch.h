// include/touch/touch.h
#ifndef INCLUDE_TOUCH_H_
#define INCLUDE_TOUCH_H_

#include <Arduino.h>
#include <Preferences.h>
#include <TFT_Touch.h>  // Bodmer's library

#include "Config.h"

extern bool recalibrateTouch;

void init_touch();
bool calibrate_touch();
bool load_touch_calibration();
bool save_touch_calibration(int hmin, int hmax, int vmin, int vmax);
void mapRawTouchToScreen(int raw_x, int raw_y, uint16_t *x, uint16_t *y);
bool readTouchMapped(uint16_t *x, uint16_t *y);

void drawCross(int x, int y, unsigned int color);
void test(void);
void drawPrompt(void);
void drawCross(int x, int y, unsigned int color);
bool getCoord();
bool setRecalibrationFlag(bool force = true);

void debugRawTouchFor30Seconds();

#endif