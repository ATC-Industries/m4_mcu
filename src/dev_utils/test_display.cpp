#include "dev_utils/test_display.h"

#include "display/backlight.h"
#include "touch/touch.h"

extern TouchScreen touch;

static uint8_t brightness = 128;  // Starting brightness level

uint16_t rainbow(uint8_t hue) {
  uint8_t r, g, b;
  if (hue < 85) {
    r = hue * 3;
    g = 255 - hue * 3;
    b = 0;
  } else if (hue < 170) {
    hue -= 85;
    r = 255 - hue * 3;
    g = 0;
    b = hue * 3;
  } else {
    hue -= 170;
    r = 0;
    g = hue * 3;
    b = 255 - hue * 3;
  }
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

#ifdef DEVELOPMENT_MODE

void drawBrightnessButtons() {
  // Top-left button for decreasing brightness
  lcd.fillRect(10, 10, 50, 50, TFT_WHITE);
  lcd.setTextColor(TFT_BLACK);
  lcd.setTextSize(3);
  lcd.setCursor(25, 20);
  lcd.print("-");

  // Top-right button for increasing brightness
  lcd.fillRect(lcd.width() - 60, 10, 50, 50, TFT_WHITE);
  lcd.setTextColor(TFT_BLACK);
  lcd.setTextSize(3);
  lcd.setCursor(lcd.width() - 45, 20);
  lcd.print("+");
}

void handleBrightnessControl() {
  uint16_t touch_x = 0, touch_y = 0, touch_z = 0;

  if (touch.isTouched()) {
    touch.readTouchPoint(&touch_x, &touch_y, &touch_z);

    // Check if "-" button was pressed
    if (touch_x > 10 && touch_x < 60 && touch_y > 10 && touch_y < 60) {
      brightness = max(0, brightness - 10);
      setBacklight(brightness);
      displayBrightnessLevel();
      delay(200);  // Debounce delay
    }

    // Check if "+" button was pressed
    if (touch_x > lcd.width() - 60 && touch_x < lcd.width() - 10 && touch_y > 10 && touch_y < 60) {
      brightness = min(255, brightness + 10);
      setBacklight(brightness);
      displayBrightnessLevel();
      delay(200);  // Debounce delay
    }
  }
}

void displayBrightnessLevel() {
  int percentage = (brightness * 100) / 255;

  // Clear the area where brightness text will appear
  lcd.fillRect((lcd.width() / 2) - 50, 10, 100, 20, TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.setCursor((lcd.width() / 2) - 40, 15);  // Position slightly lower
  lcd.printf("Brightness: %d%%", percentage);
}

void fadeInBacklightBlue() {
  lcd.fillScreen(TFT_BLACK);
  fadeBacklight(true, 1000);
  lcd.fillScreen(TFT_BLUE);
  displayBrightnessLevel();
}

void displaySolidGreen() { lcd.fillScreen(TFT_GREEN); }

void fadeOutBacklight() { fadeBacklight(false, 500); }

void fadeInBacklight() { fadeBacklight(true, 500); }

void displaySolidRed() { lcd.fillScreen(TFT_RED); }

void testBacklightLevels() {
  for (int i = 255; i >= 0; i -= 64) {
    setBacklight(i);
    lcd.fillRect(0, lcd.height() - 40, lcd.width(), 40, TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setCursor(10, lcd.height() - 30);
    lcd.printf("Backlight Level: %d", i);
    delay(500);
  }
  for (int i = 0; i <= 255; i += 64) {
    setBacklight(i);
    lcd.fillRect(0, lcd.height() - 40, lcd.width(), 40, TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setCursor(10, lcd.height() - 30);
    lcd.printf("Backlight Level: %d", i);
    delay(500);
  }
}

void showEBUColorBars() {
  int width = lcd.width() / 7;
  int height = lcd.height() / 3;

  lcd.fillRect(0 * width, 40, width, height, TFT_WHITE);
  lcd.fillRect(1 * width, 40, width, height, TFT_YELLOW);
  lcd.fillRect(2 * width, 40, width, height, TFT_CYAN);
  lcd.fillRect(3 * width, 40, width, height, TFT_GREEN);
  lcd.fillRect(4 * width, 40, width, height, TFT_MAGENTA);
  lcd.fillRect(5 * width, 40, width, height, TFT_RED);
  lcd.fillRect(6 * width, 40, width, height, TFT_BLUE);
}

void displayMovingTextAnimation() {
  static int bounce_pos = 0;
  static int bounce_dir = 1;
  static uint16_t hue = 0;

  lcd.fillRect(0, lcd.height() / 2 - 50, lcd.width(), 100, TFT_BLACK);
  lcd.setTextColor(rainbow(hue));
  lcd.setTextSize(3);
  lcd.setCursor(bounce_pos, lcd.height() / 2 - 20);
  lcd.println("ESP32-S3 RGB TEST");

  bounce_pos += bounce_dir * 10;
  if (bounce_pos >= lcd.width() - 300 || bounce_pos <= 0) {
    bounce_dir *= -1;
  }

  hue += 20;
}

void displayTouchCoordinates() {
  static uint16_t touch_x = 0, touch_y = 0, touch_z = 0;
  static bool was_touched = false;

  if (touch.isTouched()) {
    touch.readTouchPoint(&touch_x, &touch_y, &touch_z);
    lcd.fillRect(0, lcd.height() - 40, lcd.width(), 40, TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(2);
    lcd.setCursor(10, lcd.height() - 30);
    lcd.printf("Touch: X=%d Y=%d Z=%d", touch_x, touch_y, touch_z);
    was_touched = true;
  } else if (was_touched) {
    lcd.fillRect(0, lcd.height() - 40, lcd.width(), 40, TFT_BLACK);
    was_touched = false;
  }
}

void runDisplayTest() {
  fadeInBacklightBlue();
  delay(1000);

  displaySolidGreen();
  fadeOutBacklight();
  delay(250);
  fadeInBacklight();
  delay(1000);

  displaySolidRed();
  testBacklightLevels();

  lcd.fillScreen(TFT_BLACK);
  setBacklight(192);

  showEBUColorBars();
  drawBrightnessButtons();
  displayBrightnessLevel();

  while (true) {
    displayMovingTextAnimation();
    displayTouchCoordinates();
    handleBrightnessControl();
    delay(50);
  }
}

#endif  // DEVELOPMENT_MODE
