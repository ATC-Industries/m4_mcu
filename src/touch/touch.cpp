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

bool TouchScreen::readTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z) {
  const int kSamples = 3;
  uint32_t avg_x = 0, avg_y = 0, avg_z1 = 0, avg_z2 = 0;

  for (int i = 0; i < kSamples; i++) {
    avg_z1 += readChannel(kCmdZ1_);
    avg_z2 += readChannel(kCmdZ2_);
    avg_x += readChannel(kCmdX_);
    avg_y += readChannel(kCmdY_);
  }

  avg_x /= kSamples;
  avg_y /= kSamples;
  avg_z1 /= kSamples;
  avg_z2 /= kSamples;

  uint16_t pressure = avg_z1 + 4095 - avg_z2;
  if (z != nullptr) {
    *z = pressure;
  }

  if (pressure > 500) {  // Minimum pressure for valid touch
    // SWAP X and Y axes and map to screen coordinates with constraints
    int32_t mapped_x = map(avg_y, 200, 3800, 0, 800);
    int32_t mapped_y = map(avg_x, 350, 3700, 0, 480);

    // Constrain values to screen boundaries
    *x = constrain(mapped_x, 0, 799);
    *y = constrain(mapped_y, 0, 479);

    // Debug output
    Serial.printf("Raw: X=%d Y=%d -> Mapped: X=%d Y=%d (P=%d)\n", avg_x, avg_y, *x, *y, pressure);

    return true;
  }

  *x = 0;
  *y = 0;
  return false;
}

bool TouchScreen::isTouched() {
  uint16_t x, y, z;
  return readTouchPoint(&x, &y, &z);
}

uint16_t TouchScreen::interpolate(uint16_t val, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  if (in_min == in_max) return (out_min + out_max) / 2;
  long dividend = (long)(val - in_min) * (out_max - out_min);
  return out_min + dividend / (in_max - in_min);
}

void TouchScreen::mapRawToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y) {
  if (!cal_data_.valid) {
    *x = raw_x;
    *y = raw_y;
    return;
  }

  // Simple linear interpolation between calibration points
  *y = interpolate(raw_x, cal_data_.raw_x[0], cal_data_.raw_x[1], cal_data_.screen_y[0], cal_data_.screen_y[1]);
  *x = interpolate(raw_y, cal_data_.raw_y[0], cal_data_.raw_y[2], cal_data_.screen_x[0], cal_data_.screen_x[2]);
}

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

  cal_data_.valid = true;
  return saveCalibration();
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