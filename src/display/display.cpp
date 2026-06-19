#include "display/display.h"

#include "Config.h"
#include "display/backlight.h"
#include <string>

#define LOG_TAG "Display"
#include "Logging.h"

LGFX lcd;

void show_boot_splash() {
  lcd.fillScreen(TFT_SKYBLUE);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(TFT_BLACK, TFT_SKYBLUE);

  lcd.setFont(&fonts::Font4);
  lcd.drawString("M4 Sled Monitor", SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) - 24);

  const std::string version_line = std::string("V") + DEVICE_VERSION;
  lcd.setFont(&fonts::Font2);
  lcd.drawString(version_line.c_str(), SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) + 20);
}

// Initialize the display and touch
void init_display() {
  boost_rgb_drive();
  // Initialize backlight
  ledcSetup(LCD_BL_CHANNEL, LCD_BL_FREQ, LCD_BL_RESOLUTION);
  ledcAttachPin(LCD_BL_PIN, LCD_BL_CHANNEL);
  setBacklight(0);  // Start with backlight off

  // Initialize display
  if (!lcd.begin()) {
    LOGE("Display initialization failed!");
    while (1) delay(100);
  }

  // Configure display
  lcd.setRotation(SCREEN_ROTATION);  // Note: This will be adjusted based on Squareline Studio settings
  setBacklight(DEFAULT_BACKLIGHT_BRIGHTNESS);
  show_boot_splash();
}
