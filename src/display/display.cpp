#include "display/display.h"

#include "display/backlight.h"

void init_display() {
  // Initialize backlight
  ledcSetup(LCD_BL_CHANNEL, LCD_BL_FREQ, LCD_BL_RESOLUTION);
  ledcAttachPin(LCD_BL_PIN, LCD_BL_CHANNEL);
  setBacklight(0);  // Start with backlight off

  // Initialize display
  if (!lcd.begin()) {
    Serial.println("Display initialization failed!");
    while (1) delay(100);
  }

  // Configure display
  lcd.setRotation(0);  // Note: This will be adjusted based on Squareline Studio settings
  setBacklight(192);   // Set to ~75% brightness
}

LGFX lcd;