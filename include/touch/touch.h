#ifndef INCLUDE_TOUCH_H_
#define INCLUDE_TOUCH_H_

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>

class TouchScreen {
public:
  // Constructor takes pin definitions
  TouchScreen(uint8_t cs_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin);

  // Initialize the touch controller
  bool begin();

  // Read touch coordinates
  bool readTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z = nullptr);

  // Check if screen is being touched
  bool isTouched();

  // Calibration methods
  bool setCalibration(uint16_t raw_x[], uint16_t raw_y[], uint16_t x0, uint16_t y0,  // TL
                      uint16_t x1, uint16_t y1,                                      // TR
                      uint16_t x2, uint16_t y2,                                      // BR
                      uint16_t x3, uint16_t y3);                                     // BL
  bool saveCalibration();
  bool loadCalibration();

  // Recalibration flag
  static bool setRecalibrationFlag();
  static bool clearRecalibrationFlag();
  static bool checkRecalibrationFlag();

private:
  // Pin definitions
  const uint8_t kCsPin_;
  const uint8_t kMosiPin_;
  const uint8_t kMisoPin_;
  const uint8_t kSckPin_;

  // SPI settings
  static const uint32_t kSpiFreq_ = 1000000;  // 1MHz SPI clock
  SPISettings spi_settings_;

  // Command bytes for XPT2046
  static const uint8_t kCmdX_ = 0x90;   // X position  (0b10010000)
  static const uint8_t kCmdY_ = 0xD0;   // Y position  (0b11010000)
  static const uint8_t kCmdZ1_ = 0xB0;  // Z1 position (0b10110000)
  static const uint8_t kCmdZ2_ = 0xC0;  // Z2 position (0b11000000)

  struct CalibrationData {
    uint16_t raw_x[4];     // Raw X values for corners
    uint16_t raw_y[4];     // Raw Y values for corners
    uint16_t screen_x[4];  // Screen X coordinates
    uint16_t screen_y[4];  // Screen Y coordinates
    bool valid;
  } cal_data_;

  // Private helper functions
  uint16_t readChannel(uint8_t channel);
  uint16_t interpolate(uint16_t val, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
  void mapRawToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y);
};

// Declare the external global instance
extern TouchScreen touch;

#endif  // INCLUDE_TOUCH_H_