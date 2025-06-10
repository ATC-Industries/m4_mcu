#include "touch/calibration.h"

void TouchCalibration::showTouchTest(LGFX& lcd, TouchScreen& touch) {
  // Constants for the test grid
  const uint16_t kGridSize = 50;  // Size of grid squares
  const uint16_t kDotRadius = 3;  // Size of touch indicator

  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);

  // Draw calibration points
  const uint16_t margin = 25;
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
  const uint16_t margin = 25;
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
    uint16_t raw_x, raw_y, z;
    waitForRawTouch(touch, &raw_x, &raw_y, &z);
    cal_x[i] = raw_x;
    cal_y[i] = raw_y;

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
  if (!touch.setCalibration(cal_x, cal_y, margin, margin,                   // TL
                            screen_width - margin, margin,                  // TR
                            screen_width - margin, screen_height - margin,  // BR
                            margin, screen_height - margin))                // BL
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

bool TouchCalibration::waitForRawTouch(TouchScreen& touch, uint16_t* raw_x, uint16_t* raw_y, uint16_t* z) {
  // Simply call the new stable touch method
  waitForStableTouch(touch, raw_x, raw_y, z);
  return (*raw_x != 0 || *raw_y != 0);
}

void TouchCalibration::waitForStableTouch(TouchScreen& touch, uint16_t* raw_x, uint16_t* raw_y, uint16_t* z) {
  const int kStableReadings = 10;  // Number of consecutive stable readings needed
  const int kMaxDeviation = 30;    // Maximum allowed deviation between readings
  const int kTimeout = 10000;      // Timeout in milliseconds

  uint16_t readings_x[kStableReadings];
  uint16_t readings_y[kStableReadings];
  int reading_count = 0;

  uint32_t start_time = millis();

  Serial.println("Waiting for stable touch...");

  while (reading_count < kStableReadings && (millis() - start_time) < kTimeout) {
    uint16_t x, y, pressure;

    if (touch.readRawTouchPoint(&x, &y, &pressure)) {
      // For the first reading, just store it
      if (reading_count == 0) {
        readings_x[0] = x;
        readings_y[0] = y;
        reading_count = 1;
      } else {
        // Check if this reading is stable compared to all previous ones
        bool stable = true;

        for (int i = 0; i < reading_count; i++) {
          if (abs((int)x - (int)readings_x[i]) > kMaxDeviation || abs((int)y - (int)readings_y[i]) > kMaxDeviation) {
            stable = false;
            break;
          }
        }

        if (stable) {
          // Add this stable reading
          readings_x[reading_count] = x;
          readings_y[reading_count] = y;
          reading_count++;

          // Visual feedback - show progress
          Serial.printf("Stable reading %d/%d: X=%d Y=%d\n", reading_count, kStableReadings, x, y);
        } else {
          // Unstable - start over with this new reading
          Serial.println("Touch moved - restarting stability check");
          readings_x[0] = x;
          readings_y[0] = y;
          reading_count = 1;
        }
      }
    } else {
      // No touch detected - reset
      if (reading_count > 0) {
        Serial.println("Touch released - restarting");
        reading_count = 0;
      }
    }

    delay(20);  // Small delay between readings
  }

  if (reading_count >= kStableReadings) {
    // Calculate average of stable readings
    uint32_t sum_x = 0, sum_y = 0;
    for (int i = 0; i < kStableReadings; i++) {
      sum_x += readings_x[i];
      sum_y += readings_y[i];
    }

    *raw_x = sum_x / kStableReadings;
    *raw_y = sum_y / kStableReadings;
    if (z) *z = 1000;  // Dummy pressure value

    Serial.printf("Final stable reading: X=%d Y=%d\n", *raw_x, *raw_y);
  } else {
    Serial.println("Failed to get stable reading!");
    // Return last reading if we have one
    if (reading_count > 0) {
      *raw_x = readings_x[reading_count - 1];
      *raw_y = readings_y[reading_count - 1];
    } else {
      *raw_x = 0;
      *raw_y = 0;
    }
  }
}
