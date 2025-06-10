#include "touch/touch.h"

#include <math.h>

// Create the global instance
TouchScreen touch(38, 11, 13, 12);  // CS, MOSI, MISO, SCK
static Preferences touchPrefs;

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

  // Send command byte
  SPI.transfer(channel);
  // Read 16-bit result
  uint16_t data = SPI.transfer16(0x00);

  digitalWrite(kCsPin_, HIGH);
  SPI.endTransaction();

  // XPT2046 returns 12-bit data in bits 14-3
  return data >> 3;
}

bool TouchScreen::readTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z) {
  uint16_t raw_x, raw_y, raw_z;

  if (!readRawTouchPoint(&raw_x, &raw_y, &raw_z)) {
    *x = 0;
    *y = 0;
    if (z) *z = 0;
    return false;
  }

  if (z) *z = raw_z;

  // Apply calibration
  if (cal_data_.valid) {
    // Use bilinear interpolation for better corner accuracy
    applyBilinearInterpolation(raw_x, raw_y, x, y);
  } else {
    // Simple linear mapping as fallback
    *x = map(raw_x, 200, 3800, 0, 800);
    *y = map(raw_y, 350, 3700, 0, 480);
  }

  // Constrain to screen boundaries
  *x = constrain(*x, 0, 799);
  *y = constrain(*y, 0, 479);

  return true;
}

bool TouchScreen::readRawTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z) {
  const int kSamples = 32;
  const int kPressureThreshold = 150;
  const int kDiscardCount = 6;
  const int kSpikeThreshold = 100;  // Raw units spike threshold

  uint16_t samples_x[kSamples];
  uint16_t samples_y[kSamples];
  uint16_t samples_z1[kSamples];
  uint16_t samples_z2[kSamples];
  int valid_samples = 0;

  // First pass: collect samples and detect spikes
  uint16_t first_x = 0, first_y = 0;
  bool has_reference = false;

  for (int i = 0; i < kSamples; i++) {
    // Read pressure first
    uint16_t z1 = readChannel(kCmdZ1_);
    uint16_t z2 = readChannel(kCmdZ2_);
    uint16_t pressure = z1 + 4095 - z2;

    if (pressure > 100) {  // Basic pressure check
      uint16_t curr_x = readChannel(kCmdX_);
      uint16_t curr_y = readChannel(kCmdY_);

      // Spike detection
      bool is_spike = false;
      if (has_reference) {
        int dx = abs((int)curr_x - (int)first_x);
        int dy = abs((int)curr_y - (int)first_y);
        if (dx > kSpikeThreshold || dy > kSpikeThreshold) {
          is_spike = true;
          Serial.printf("Raw spike filtered: X:%d->%d Y:%d->%d\n", first_x, curr_x, first_y, curr_y);
        }
      } else {
        // First valid reading becomes reference
        first_x = curr_x;
        first_y = curr_y;
        has_reference = true;
      }

      // Only store non-spike samples
      if (!is_spike && valid_samples < kSamples) {
        samples_x[valid_samples] = curr_x;
        samples_y[valid_samples] = curr_y;
        samples_z1[valid_samples] = z1;
        samples_z2[valid_samples] = z2;
        valid_samples++;
      }
    }

    delayMicroseconds(100);
  }

  // Need minimum valid samples
  if (valid_samples < (kDiscardCount * 2 + 1)) {
    return false;
  }

  // Sort valid samples
  sortArray(samples_x, valid_samples);
  sortArray(samples_y, valid_samples);
  sortArray(samples_z1, valid_samples);
  sortArray(samples_z2, valid_samples);

  // Calculate average excluding outliers
  int start_idx = min(kDiscardCount, valid_samples / 4);
  int end_idx = max(valid_samples - kDiscardCount, valid_samples * 3 / 4);
  int count = end_idx - start_idx;

  if (count <= 0) {
    return false;
  }

  uint32_t sum_x = 0, sum_y = 0, sum_z1 = 0, sum_z2 = 0;
  for (int i = start_idx; i < end_idx; i++) {
    sum_x += samples_x[i];
    sum_y += samples_y[i];
    sum_z1 += samples_z1[i];
    sum_z2 += samples_z2[i];
  }

  uint16_t avg_x = sum_x / count;
  uint16_t avg_y = sum_y / count;
  uint16_t avg_z1 = sum_z1 / count;
  uint16_t avg_z2 = sum_z2 / count;

  uint16_t pressure = avg_z1 + 4095 - avg_z2;
  if (z) *z = pressure;

  if (pressure < kPressureThreshold) {
    return false;
  }

  *x = avg_x;
  *y = avg_y;

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

void TouchScreen::applyBilinearInterpolation(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y) {
  // Bilinear interpolation using the 4 calibration points
  // This handles non-linear behavior near corners much better

  // Find normalized position (0-1) within the calibration rectangle
  float norm_x = (float)(raw_x - cal_data_.raw_min_x) / (cal_data_.raw_max_x - cal_data_.raw_min_x);
  float norm_y = (float)(raw_y - cal_data_.raw_min_y) / (cal_data_.raw_max_y - cal_data_.raw_min_y);

  // Clamp to 0-1 range
  norm_x = constrain(norm_x, 0.0f, 1.0f);
  norm_y = constrain(norm_y, 0.0f, 1.0f);

  // Apply edge correction factor (reduces sensitivity near edges)
  if (cal_data_.edge_correction_factor > 0) {
    // Apply non-linear transformation to reduce edge effects
    float edge_factor = cal_data_.edge_correction_factor;
    norm_x = 0.5f + (norm_x - 0.5f) * (1.0f - edge_factor * (0.5f - fabs(norm_x - 0.5f)));
    norm_y = 0.5f + (norm_y - 0.5f) * (1.0f - edge_factor * (0.5f - fabs(norm_y - 0.5f)));
  }

  // Bilinear interpolation
  // Order: TL(0), TR(1), BR(2), BL(3)
  float x_top = cal_data_.screen_x[0] + norm_x * (cal_data_.screen_x[1] - cal_data_.screen_x[0]);
  float x_bot = cal_data_.screen_x[3] + norm_x * (cal_data_.screen_x[2] - cal_data_.screen_x[3]);
  float x_final = x_top + norm_y * (x_bot - x_top);

  float y_left = cal_data_.screen_y[0] + norm_y * (cal_data_.screen_y[3] - cal_data_.screen_y[0]);
  float y_right = cal_data_.screen_y[1] + norm_y * (cal_data_.screen_y[2] - cal_data_.screen_y[1]);
  float y_final = y_left + norm_x * (y_right - y_left);

  *x = (uint16_t)(x_final + 0.5f);
  *y = (uint16_t)(y_final + 0.5f);
}

bool TouchScreen::calculateCalibrationMatrix() {
  if (!cal_data_.valid) return false;

  // Calculate the bounding box of raw values
  cal_data_.raw_min_x = min(min(cal_data_.raw_x[0], cal_data_.raw_x[1]), min(cal_data_.raw_x[2], cal_data_.raw_x[3]));
  cal_data_.raw_max_x = max(max(cal_data_.raw_x[0], cal_data_.raw_x[1]), max(cal_data_.raw_x[2], cal_data_.raw_x[3]));
  cal_data_.raw_min_y = min(min(cal_data_.raw_y[0], cal_data_.raw_y[1]), min(cal_data_.raw_y[2], cal_data_.raw_y[3]));
  cal_data_.raw_max_y = max(max(cal_data_.raw_y[0], cal_data_.raw_y[1]), max(cal_data_.raw_y[2], cal_data_.raw_y[3]));

  // Set edge correction factor (0.0 to 0.5, higher = more correction)
  cal_data_.edge_correction_factor = 0.3f;

  // Still calculate linear matrix as fallback
  // Using all 4 points with least squares
  float sum_x = 0, sum_y = 0, sum_X = 0, sum_Y = 0;
  float sum_xx = 0, sum_xy = 0, sum_yy = 0;
  float sum_xX = 0, sum_yX = 0, sum_xY = 0, sum_yY = 0;

  for (int i = 0; i < 4; i++) {
    float x = cal_data_.raw_x[i];
    float y = cal_data_.raw_y[i];
    float X = cal_data_.screen_x[i];
    float Y = cal_data_.screen_y[i];

    sum_x += x;
    sum_y += y;
    sum_X += X;
    sum_Y += Y;
    sum_xx += x * x;
    sum_xy += x * y;
    sum_yy += y * y;
    sum_xX += x * X;
    sum_yX += y * X;
    sum_xY += x * Y;
    sum_yY += y * Y;
  }

  // Calculate matrix for linear transformation (used as fallback)
  float n = 4.0f;
  float det = n * (sum_xx * sum_yy - sum_xy * sum_xy) - sum_x * (sum_x * sum_yy - sum_y * sum_xy) +
              sum_y * (sum_x * sum_xy - sum_y * sum_xx);

  if (fabs(det) < 1.0f) {
    Serial.println("Calibration points are invalid!");
    return false;
  }

  // Store linear transformation coefficients
  cal_data_.matrix[0] = (n * sum_xX - sum_x * sum_X) / (n * sum_xx - sum_x * sum_x);
  cal_data_.matrix[1] =
      (n * sum_yX - sum_y * sum_X - cal_data_.matrix[0] * (n * sum_xy - sum_x * sum_y)) / (n * sum_yy - sum_y * sum_y);
  cal_data_.matrix[2] = (sum_X - cal_data_.matrix[0] * sum_x - cal_data_.matrix[1] * sum_y) / n;

  cal_data_.matrix[3] = (n * sum_xY - sum_x * sum_Y) / (n * sum_xx - sum_x * sum_x);
  cal_data_.matrix[4] =
      (n * sum_yY - sum_y * sum_Y - cal_data_.matrix[3] * (n * sum_xy - sum_x * sum_y)) / (n * sum_yy - sum_y * sum_y);
  cal_data_.matrix[5] = (sum_Y - cal_data_.matrix[3] * sum_x - cal_data_.matrix[4] * sum_y) / n;

  // Debug output
  Serial.println("Calibration data:");
  Serial.printf("Raw bounds: X[%d-%d] Y[%d-%d]\n", cal_data_.raw_min_x, cal_data_.raw_max_x, cal_data_.raw_min_y,
                cal_data_.raw_max_y);
  Serial.printf("Edge correction: %.2f\n", cal_data_.edge_correction_factor);

  // Test calibration accuracy
  Serial.println("Testing calibration accuracy:");
  for (int i = 0; i < 4; i++) {
    uint16_t test_x, test_y;
    applyBilinearInterpolation(cal_data_.raw_x[i], cal_data_.raw_y[i], &test_x, &test_y);
    int error_x = abs((int)test_x - (int)cal_data_.screen_x[i]);
    int error_y = abs((int)test_y - (int)cal_data_.screen_y[i]);
    Serial.printf("Point %d: Expected(%d,%d) Got(%d,%d) Error(%d,%d)\n", i, cal_data_.screen_x[i],
                  cal_data_.screen_y[i], test_x, test_y, error_x, error_y);
  }

  return true;
}
bool TouchScreen::setCalibration(uint16_t raw_x[], uint16_t raw_y[], uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                                 uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3) {
  // Store calibration points
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

  cal_data_.valid = true;

  // Calculate calibration parameters
  if (!calculateCalibrationMatrix()) {
    cal_data_.valid = false;
    return false;
  }

  return saveCalibration();
}

void TouchScreen::mapRawToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y) {
  if (!cal_data_.valid) {
    *x = raw_x;
    *y = raw_y;
    return;
  }

  // Apply transformation matrix
  float fx = cal_data_.matrix[0] * raw_x + cal_data_.matrix[1] * raw_y + cal_data_.matrix[2];
  float fy = cal_data_.matrix[3] * raw_x + cal_data_.matrix[4] * raw_y + cal_data_.matrix[5];

  // Round and constrain
  *x = static_cast<uint16_t>(constrain(fx + 0.5f, 0.0f, 799.0f));
  *y = static_cast<uint16_t>(constrain(fy + 0.5f, 0.0f, 479.0f));
}

bool TouchScreen::saveCalibration() {
  Serial.println("Entered saveCalibration()");

  if (!touchPrefs.begin("touch_cal", false)) {
    Serial.println("Failed to open prefs for saving");
    return false;
  }

  size_t written = touchPrefs.putBytes("cal_data", &cal_data_, sizeof(CalibrationData));
  touchPrefs.end();

  if (written != sizeof(CalibrationData)) {
    Serial.printf("Failed to save all calibration bytes (%u of %u)\n", written, sizeof(CalibrationData));
    return false;
  }
  Serial.printf("Wrote %u of %u bytes\n", written, sizeof(CalibrationData));

  Serial.println("Calibration data saved successfully");
  return true;
}

bool TouchScreen::loadCalibration() {
  if (!touchPrefs.begin("touch_cal", true)) {
    return false;
  }

  size_t size = touchPrefs.getBytes("cal_data", &cal_data_, sizeof(CalibrationData));
  touchPrefs.end();

  return (size == sizeof(CalibrationData) && cal_data_.valid);
}

bool TouchScreen::setRecalibrationFlag() {
  if (!touchPrefs.begin("touch_cal", false)) {
    return false;
  }
  bool success = touchPrefs.putBool("need_cal", true);
  touchPrefs.end();
  return success;
}

bool TouchScreen::clearRecalibrationFlag() {
  if (!touchPrefs.begin("touch_cal", false)) {
    return false;
  }
  bool success = touchPrefs.putBool("need_cal", false);
  touchPrefs.end();
  return success;
}

bool TouchScreen::checkRecalibrationFlag() {
  if (!touchPrefs.begin("touch_cal", true)) {
    return false;
  }
  bool need_cal = touchPrefs.getBool("need_cal", false);
  touchPrefs.end();
  return need_cal;
}