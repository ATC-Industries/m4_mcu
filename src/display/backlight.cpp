#include "display/backlight.h"

static uint8_t current_brightness = 200;  // Default brightness (0-255)

void setBacklight(uint8_t brightness) {
  current_brightness = brightness;  // Store the current brightness
  ledcWrite(LCD_BL_CHANNEL, brightness);
}
void fadeBacklight(bool fadeIn, int duration) {
  int steps = 256;
  int delayTime = duration / steps;
  for (int i = 0; i < steps; i++) {
    int brightness = fadeIn ? i : (255 - i);
    setBacklight(brightness);
    delay(delayTime);
  }
}
uint8_t getBacklight() { return current_brightness; }
