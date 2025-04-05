#include "touch/calibration.h"

void TouchCalibration::showTouchTest(LGFX& lcd, TouchScreen& touch) {
  // Constants for the test grid
  const uint16_t kGridSize = 50;  // Size of grid squares
  const uint16_t kDotRadius = 3;  // Size of touch indicator

  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);

  // Draw calibration points
  const uint16_t margin = 50;
  drawTarget(lcd, margin, margin);                               // Top-left
  drawTarget(lcd, lcd.width() - margin, margin);                 // Top-right
  drawTarget(lcd, lcd.width() - margin, lcd.height() - margin);  // Bottom-right
  drawTarget(lcd, margin, lcd.height() - margin);                // Bottom-left

  // Draw grid
  lcd.setTextSize(1);
  for (int x = 0; x < lcd.width(); x += kGridSize) {
    lcd.drawLine(x, 0, x, lcd.height(), TFT_DARKGREY);
    if (x % 100 == 0) {  // Label every 100 pixels
      lcd.setCursor(x + 2, 2);
      lcd.printf("%d", x);
    }
  }
  for (int y = 0; y < lcd.height(); y += kGridSize) {
    lcd.drawLine(0, y, lcd.width(), y, TFT_DARKGREY);
    if (y % 100 == 0) {  // Label every 100 pixels
      lcd.setCursor(2, y + 2);
      lcd.printf("%d", y);
    }
  }

  // Instructions
  lcd.setTextSize(2);
  lcd.setCursor(10, 10);
  lcd.println("Touch Test Mode");
  lcd.setCursor(10, 30);
  lcd.println("Touch calibration points to verify");

  const int kTrailLength = 10;  // Number of previous points to show
  uint16_t trail_x[kTrailLength] = {0};
  uint16_t trail_y[kTrailLength] = {0};
  int trail_index = 0;

  uint32_t start_time = millis();
  while (millis() - start_time < 30000) {  // Run for 30 seconds
    uint16_t x, y, z;
    if (touch.readTouchPoint(&x, &y, &z)) {
      // Store point in trail
      trail_x[trail_index] = x;
      trail_y[trail_index] = y;
      trail_index = (trail_index + 1) % kTrailLength;

      // Clear coordinate display area
      lcd.fillRect(10, lcd.height() - 60, lcd.width() - 20, 50, TFT_BLACK);

      // Show coordinates
      lcd.setTextSize(2);
      lcd.setCursor(10, lcd.height() - 50);
      lcd.printf("X: %3d  Y: %3d", x, y);
      lcd.setCursor(10, lcd.height() - 25);
      lcd.printf("Pressure: %4d", z);

      // Draw trail with fading
      for (int i = 0; i < kTrailLength; i++) {
        if (trail_x[i] != 0 || trail_y[i] != 0) {
          int age = (kTrailLength + trail_index - i) % kTrailLength;
          uint16_t color = lcd.color565(255 - (age * 25), 255 - (age * 25), 255 - (age * 25));
          lcd.fillCircle(trail_x[i], trail_y[i], kDotRadius, color);
        }
      }
    } else {
      // Clear old points when touch released
      if (trail_x[trail_index] != 0 || trail_y[trail_index] != 0) {
        trail_x[trail_index] = 0;
        trail_y[trail_index] = 0;
        trail_index = (trail_index + 1) % kTrailLength;
      }
    }
    delay(10);
  }

  // Clear screen when done
  lcd.fillScreen(TFT_BLACK);
}

bool TouchCalibration::waitForTouch(TouchScreen& touch, uint16_t* x, uint16_t* y, uint16_t* z) {
  return touch.readTouchPoint(x, y, z);
}

bool TouchCalibration::waitForRelease(TouchScreen& touch) {
  uint16_t x, y, z;
  while (touch.readTouchPoint(&x, &y, &z)) {
    delay(10);
  }
  return true;
}

void TouchCalibration::drawTarget(LGFX& lcd, uint16_t x, uint16_t y) {
  lcd.drawCircle(x, y, kTargetSize, TFT_RED);
  lcd.drawCircle(x, y, kTargetSize / 2, TFT_RED);
  lcd.drawLine(x - kTargetSize, y, x + kTargetSize, y, TFT_RED);
  lcd.drawLine(x, y - kTargetSize, x, y + kTargetSize, TFT_RED);
}

void TouchCalibration::showCalibrationPoint(LGFX& lcd, uint16_t x, uint16_t y, const char* message) {
  lcd.fillScreen(TFT_BLACK);
  drawTarget(lcd, x, y);

  clearTextArea(lcd);
  lcd.setCursor(10, lcd.height() / 2);
  lcd.print(message);
}

void TouchCalibration::clearTextArea(LGFX& lcd) {
  lcd.fillRect(0, lcd.height() / 2 - kTextHeight / 2, lcd.width(), kTextHeight, TFT_BLACK);
}

bool TouchCalibration::runCalibration(LGFX& lcd, TouchScreen& touch) {
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);

  // Calibration points in screen coordinates
  const uint16_t margin = 50;
  const uint16_t screen_width = static_cast<uint16_t>(lcd.width());
  const uint16_t screen_height = static_cast<uint16_t>(lcd.height());

  struct CalPoint {
    uint16_t x;
    uint16_t y;
    const char* msg;
  } points[] = {{margin, margin, "Touch the target in top-left"},
                {static_cast<uint16_t>(screen_width - margin), margin, "Touch the target in top-right"},
                {static_cast<uint16_t>(screen_width - margin), static_cast<uint16_t>(screen_height - margin),
                 "Touch the target in bottom-right"},
                {margin, static_cast<uint16_t>(screen_height - margin), "Touch the target in bottom-left"}};

  // Collect calibration points
  uint16_t cal_x[4], cal_y[4];  // Raw touch values for each corner

  for (int i = 0; i < 4; i++) {
    showCalibrationPoint(lcd, points[i].x, points[i].y, points[i].msg);

    // Wait for any previous touch to be released
    waitForRelease(touch);
    delay(kDebounceDelay);

    // Get touch point
    uint16_t x, y, z;
    while (!waitForTouch(touch, &x, &y, &z)) {
      delay(10);
    }

    // Store raw values
    cal_x[i] = x;
    cal_y[i] = y;

    // Show success
    clearTextArea(lcd);
    lcd.setCursor(10, lcd.height() / 2);
    lcd.print("Point recorded!");
    delay(500);

    // Wait for release before next point
    waitForRelease(touch);
    delay(kDebounceDelay);
  }

  // Save calibration data
  if (!touch.setCalibration(cal_x, cal_y, points[0].x, points[0].y,  // TL
                            points[1].x, points[1].y,                // TR
                            points[2].x, points[2].y,                // BR
                            points[3].x, points[3].y))               // BL
  {
    lcd.fillScreen(TFT_BLACK);
    lcd.setCursor(10, lcd.height() / 2);
    lcd.print("Calibration save failed!");
    delay(2000);
    return false;
  }

  // Run touch test to verify calibration
  showTouchTest(lcd, touch);

  return true;
}