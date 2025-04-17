#include "touch/touch.h"

// Create the global instance
TouchScreen touch(38, 11, 13, 12);  // CS, MOSI, MISO, SCK

TouchScreen::TouchScreen(uint8_t cs_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin)
    : kCsPin_(cs_pin),
      kMosiPin_(mosi_pin),
      kMisoPin_(miso_pin),
      kSckPin_(sck_pin),
      spi_settings_(kSpiFreq_, MSBFIRST, SPI_MODE0) {
  cal_data_.valid = false;  // Initialize calibration as invalid
}

bool TouchScreen::begin() {
  Serial.println("Initializing TouchScreen...");

  pinMode(kCsPin_, OUTPUT);
  digitalWrite(kCsPin_, HIGH);

  Serial.printf("Touch pins - CS:%d MOSI:%d MISO:%d SCK:%d\n", kCsPin_, kMosiPin_, kMisoPin_, kSckPin_);

  SPI.begin(kSckPin_, kMisoPin_, kMosiPin_);
  return true;
}

uint16_t TouchScreen::readChannel(uint8_t channel) {
  SPI.beginTransaction(spi_settings_);
  digitalWrite(kCsPin_, LOW);
  delayMicroseconds(100);

  SPI.transfer(channel);
  delayMicroseconds(100);
  uint16_t data = SPI.transfer16(0x00);

  digitalWrite(kCsPin_, HIGH);
  SPI.endTransaction();

  return data >> 3;  // 12-bit resolution
}

// bool TouchScreen::readTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z) {
//   const int kSamples = 3;
//   uint32_t avg_x = 0, avg_y = 0, avg_z1 = 0, avg_z2 = 0;

//   for (int i = 0; i < kSamples; i++) {
//     avg_z1 += readChannel(kCmdZ1_);
//     avg_z2 += readChannel(kCmdZ2_);
//     avg_x += readChannel(kCmdX_);
//     avg_y += readChannel(kCmdY_);
//   }

//   avg_x /= kSamples;
//   avg_y /= kSamples;
//   avg_z1 /= kSamples;
//   avg_z2 /= kSamples;

//   uint16_t pressure = avg_z1 + 4095 - avg_z2;
//   if (z != nullptr) {
//     *z = pressure;
//   }

//   if (pressure > 500) {  // Minimum pressure for valid touch
//     // SWAP X and Y axes and map to screen coordinates with constraints
//     int32_t mapped_x = map(avg_y, 200, 3800, 0, 800);
//     int32_t mapped_y = map(avg_x, 350, 3700, 0, 480);

//     // Constrain values to screen boundaries
//     *x = constrain(mapped_x, 0, 799);
//     *y = constrain(mapped_y, 0, 479);

//     // Debug output
//     Serial.printf("Raw: X=%d Y=%d -> Mapped: X=%d Y=%d (P=%d)\n", avg_x, avg_y, *x, *y, pressure);

//     return true;
//   }

//   *x = 0;
//   *y = 0;
//   return false;
// }
/////////////
bool TouchScreen::readTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z) {
  const int kSamples = 10;  // Increased sample count for better averaging
  const int kPressureThreshold = 1000;
  const int kJitterThreshold = 100;  // Maximum allowed point movement to be considered the same point

  static uint16_t last_x = 0, last_y = 0;
  static uint16_t stable_x = 0, stable_y = 0;
  static uint8_t stable_count = 0;

  // Arrays to store samples
  uint16_t samples_x[kSamples];
  uint16_t samples_y[kSamples];
  uint16_t samples_z1[kSamples];
  uint16_t samples_z2[kSamples];

  // Take multiple samples
  for (int i = 0; i < kSamples; i++) {
    samples_z1[i] = readChannel(kCmdZ1_);
    samples_z2[i] = readChannel(kCmdZ2_);
    samples_x[i] = readChannel(kCmdX_);
    samples_y[i] = readChannel(kCmdY_);
    delayMicroseconds(100);  // Small delay between samples
  }

  // Sort samples to remove outliers
  sortArray(samples_x, kSamples);
  sortArray(samples_y, kSamples);
  sortArray(samples_z1, kSamples);
  sortArray(samples_z2, kSamples);

  // Use median values (more robust than mean)
  uint16_t med_x = samples_x[kSamples / 2];
  uint16_t med_y = samples_y[kSamples / 2];
  uint16_t med_z1 = samples_z1[kSamples / 2];
  uint16_t med_z2 = samples_z2[kSamples / 2];

  // Calculate pressure
  uint16_t pressure = med_z1 + 4095 - med_z2;
  if (z != nullptr) {
    *z = pressure;
  }

  // Check pressure threshold
  if (pressure <= kPressureThreshold) {
    *x = 0;
    *y = 0;
    stable_count = 0;
    return false;
  }

  // Apply coordinate transformations (swap X/Y and map to screen)
  // Adjust these map ranges based on your actual touchscreen characteristics
  int32_t mapped_x = map(med_y, 200, 3800, 0, 800);
  int32_t mapped_y = map(med_x, 350, 3700, 0, 480);

  // uint16_t mapped_x, mapped_y;
  // mapRawToScreen(med_x, med_y, &mapped_x, &mapped_y);

  // Constrain values to screen boundaries
  mapped_x = constrain(mapped_x, 0, 799);
  mapped_y = constrain(mapped_y, 0, 479);

  // Apply stability checking - if point hasn't moved much, keep previous stable point
  if (stable_count > 0 && abs(mapped_x - last_x) < kJitterThreshold && abs(mapped_y - last_y) < kJitterThreshold) {
    // Slowly adjust stable point toward current position (dampening)
    stable_x = (stable_x * 3 + mapped_x) / 4;
    stable_y = (stable_y * 3 + mapped_y) / 4;

    if (stable_count < 255) stable_count++;
  } else {
    // Point moved significantly, reset stability
    stable_x = mapped_x;
    stable_y = mapped_y;
    stable_count = 1;
  }

  // Save last position
  last_x = mapped_x;
  last_y = mapped_y;

  // Return the stable point instead of raw point
  *x = stable_x;
  *y = stable_y;

  // Debug output
  Serial.printf("Raw: X=%d Y=%d -> Mapped: X=%d Y=%d -> Stable: X=%d Y=%d (P=%d)\n", med_x, med_y, mapped_x, mapped_y,
                stable_x, stable_y, pressure);

  return true;
}

// Helper function to sort an array (insertion sort)
void TouchScreen::sortArray(uint16_t array[], int size) {
  for (int i = 1; i < size; i++) {
    uint16_t key = array[i];
    int j = i - 1;

    while (j >= 0 && array[j] > key) {
      array[j + 1] = array[j];
      j--;
    }

    array[j + 1] = key;
  }
}

/////////////

bool TouchScreen::isTouched() {
  uint16_t x, y, z;
  return readTouchPoint(&x, &y, &z);
}

uint16_t TouchScreen::interpolate(uint16_t val, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  if (in_min == in_max) return (out_min + out_max) / 2;
  long dividend = (long)(val - in_min) * (out_max - out_min);
  return out_min + dividend / (in_max - in_min);
}

// void TouchScreen::mapRawToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y) {
//   if (!cal_data_.valid) {
//     *x = raw_x;
//     *y = raw_y;
//     return;
//   }

//   // Simple linear interpolation between calibration points
//   *y = interpolate(raw_x, cal_data_.raw_x[0], cal_data_.raw_x[1], cal_data_.screen_y[0], cal_data_.screen_y[1]);
//   *x = interpolate(raw_y, cal_data_.raw_y[0], cal_data_.raw_y[2], cal_data_.screen_x[0], cal_data_.screen_x[2]);
// }

// bool TouchScreen::setCalibration(uint16_t raw_x[], uint16_t raw_y[], uint16_t x0, uint16_t y0, uint16_t x1, uint16_t
// y1,
//                                  uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3) {
//   memcpy(cal_data_.raw_x, raw_x, sizeof(uint16_t) * 4);
//   memcpy(cal_data_.raw_y, raw_y, sizeof(uint16_t) * 4);

//   cal_data_.screen_x[0] = x0;
//   cal_data_.screen_y[0] = y0;  // TL
//   cal_data_.screen_x[1] = x1;
//   cal_data_.screen_y[1] = y1;  // TR
//   cal_data_.screen_x[2] = x2;
//   cal_data_.screen_y[2] = y2;  // BR
//   cal_data_.screen_x[3] = x3;
//   cal_data_.screen_y[3] = y3;  // BL

//   cal_data_.valid = true;
//   return saveCalibration();
// }

// Add this method to calculate the transformation matrix
bool TouchScreen::calculateCalibrationMatrix() {
  if (!cal_data_.valid) return false;

  // For a resistive touch panel, we need to calculate the transformation
  // matrix that converts touch coordinates to screen coordinates.

  // Variables for matrix calculation
  float x[4], y[4], X[4], Y[4];

  // Copy calibration points to floating point arrays
  for (int i = 0; i < 4; i++) {
    x[i] = cal_data_.raw_x[i];
    y[i] = cal_data_.raw_y[i];
    X[i] = cal_data_.screen_x[i];
    Y[i] = cal_data_.screen_y[i];
  }

  // Calculate determinant of coefficient matrix
  float det = (x[0] - x[2]) * (y[1] - y[2]) - (x[1] - x[2]) * (y[0] - y[2]);

  // If determinant is zero, calibration points are collinear
  if (abs(det) < 0.1) return false;

  // Calculate matrix coefficients
  cal_data_.matrix[0] = ((X[0] - X[2]) * (y[1] - y[2]) - (X[1] - X[2]) * (y[0] - y[2])) / det;
  cal_data_.matrix[1] = ((x[0] - x[2]) * (X[1] - X[2]) - (X[0] - X[2]) * (x[1] - x[2])) / det;
  cal_data_.matrix[2] =
      (y[0] * (x[2] * X[1] - x[1] * X[2]) + y[1] * (x[0] * X[2] - x[2] * X[0]) + y[2] * (x[1] * X[0] - x[0] * X[1])) /
      det;
  cal_data_.matrix[3] = ((Y[0] - Y[2]) * (y[1] - y[2]) - (Y[1] - Y[2]) * (y[0] - y[2])) / det;
  cal_data_.matrix[4] = ((x[0] - x[2]) * (Y[1] - Y[2]) - (Y[0] - Y[2]) * (x[1] - x[2])) / det;
  cal_data_.matrix[5] =
      (y[0] * (x[2] * Y[1] - x[1] * Y[2]) + y[1] * (x[0] * Y[2] - x[2] * Y[0]) + y[2] * (x[1] * Y[0] - x[0] * Y[1])) /
      det;

  return true;
}

// Update your setCalibration method to call calculateCalibrationMatrix
bool TouchScreen::setCalibration(uint16_t raw_x[], uint16_t raw_y[], uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                                 uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3) {
  memcpy(cal_data_.raw_x, raw_x, sizeof(uint16_t) * 4);
  memcpy(cal_data_.raw_y, raw_y, sizeof(uint16_t) * 4);

  cal_data_.screen_x[0] = x0;
  cal_data_.screen_y[0] = y0;  // TL
  cal_data_.screen_x[1] = x1;
  cal_data_.screen_y[1] = y1;  // TR
  cal_data_.screen_x[2] = x2;
  cal_data_.screen_y[2] = y2;  // BR
  cal_data_.screen_x[3] = x3;
  cal_data_.screen_y[3] = y3;  // BL

  // Calculate the transformation matrix
  if (!calculateCalibrationMatrix()) {
    return false;
  }

  cal_data_.valid = true;
  return saveCalibration();
}

// Replace your mapRawToScreen method with this one
void TouchScreen::mapRawToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y) {
  if (!cal_data_.valid) {
    *x = raw_x;
    *y = raw_y;
    return;
  }

  // Apply matrix transformation
  float tx = cal_data_.matrix[0] * raw_x + cal_data_.matrix[1] * raw_y + cal_data_.matrix[2];
  float ty = cal_data_.matrix[3] * raw_x + cal_data_.matrix[4] * raw_y + cal_data_.matrix[5];

  // Convert to integers and constrain to screen boundaries
  *x = constrain(static_cast<uint16_t>(tx + 0.5), 0, 799);
  *y = constrain(static_cast<uint16_t>(ty + 0.5), 0, 479);
}

bool TouchScreen::saveCalibration() {
  Preferences prefs;
  if (!prefs.begin("touch_cal", false)) {
    return false;
  }

  bool success = prefs.putBytes("cal_data", &cal_data_, sizeof(CalibrationData));
  prefs.end();
  return success;
}

bool TouchScreen::loadCalibration() {
  Preferences prefs;
  if (!prefs.begin("touch_cal", true)) {
    return false;
  }

  size_t size = prefs.getBytes("cal_data", &cal_data_, sizeof(CalibrationData));
  prefs.end();

  return (size == sizeof(CalibrationData) && cal_data_.valid);
}

bool TouchScreen::setRecalibrationFlag() {
  Preferences prefs;
  if (!prefs.begin("touch_cal", false)) {
    return false;
  }
  bool success = prefs.putBool("need_cal", true);
  prefs.end();
  return success;
}

bool TouchScreen::clearRecalibrationFlag() {
  Preferences prefs;
  if (!prefs.begin("touch_cal", false)) {
    return false;
  }
  bool success = prefs.putBool("need_cal", false);
  prefs.end();
  return success;
}

bool TouchScreen::checkRecalibrationFlag() {
  Preferences prefs;
  if (!prefs.begin("touch_cal", true)) {
    return false;
  }
  bool need_cal = prefs.getBool("need_cal", false);
  prefs.end();
  return need_cal;
}