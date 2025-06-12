#ifndef INCLUDE_TOUCH_H_
#define INCLUDE_TOUCH_H_

#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>

void init_touch();

class TouchScreen {
public:
  TouchScreen(uint8_t cs_pin, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin);
  bool begin();
  bool readTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z = nullptr);
  bool readRawTouchPoint(uint16_t* x, uint16_t* y, uint16_t* z);
  bool isTouched();

  bool setCalibration(uint16_t raw_x[], uint16_t raw_y[], uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3);
  bool saveCalibration();
  bool loadCalibration();

  static bool setRecalibrationFlag();
  static bool clearRecalibrationFlag();
  static bool checkRecalibrationFlag();

private:
  void sortArray(uint16_t array[], int size);
  bool calculateCalibrationMatrix();

  const uint8_t kCsPin_;
  const uint8_t kMosiPin_;
  const uint8_t kMisoPin_;
  const uint8_t kSckPin_;

  static const uint32_t kSpiFreq_ = 1000000;
  SPISettings spi_settings_;

  static const uint8_t kCmdX_ = 0xD0;
  static const uint8_t kCmdY_ = 0x90;
  static const uint8_t kCmdZ1_ = 0xB0;
  static const uint8_t kCmdZ2_ = 0xC0;

  struct CalibrationData {
    bool valid;
    uint16_t raw_x[4];  // TL, TR, BR, BL
    uint16_t raw_y[4];
    uint16_t screen_x[4];
    uint16_t screen_y[4];
    float matrix[6];
    // Additional parameters for edge correction
    uint16_t raw_min_x, raw_max_x;
    uint16_t raw_min_y, raw_max_y;
    float edge_correction_factor;
  } cal_data_;

  uint16_t readChannel(uint8_t channel);
  void mapRawToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y);
  void applyBilinearInterpolation(uint16_t raw_x, uint16_t raw_y, uint16_t* x, uint16_t* y);
};

extern TouchScreen touch;

#endif