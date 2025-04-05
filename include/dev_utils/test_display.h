#ifndef TEST_DISPLAY_H
#define TEST_DISPLAY_H

#include <Arduino.h>

#include "display/display.h"

uint16_t rainbow(uint8_t hue);

#ifdef DEVELOPMENT_MODE
void runDisplayTest();

void fadeInBacklightBlue();
void displaySolidGreen();
void fadeOutBacklight();
void fadeInBacklight();
void displaySolidRed();
void testBacklightLevels();
void showEBUColorBars();
void displayMovingTextAnimation();
void displayTouchCoordinates();

// New functions for brightness control
void drawBrightnessButtons();
void handleBrightnessControl();
void displayBrightnessLevel();  // Display brightness level as a percentage

#endif

#endif  // TEST_DISPLAY_H
