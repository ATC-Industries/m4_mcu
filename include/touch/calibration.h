#ifndef TOUCH_CALIBRATION_H_
#define TOUCH_CALIBRATION_H_

#include <Arduino.h>
#include <Preferences.h>

namespace touch {

/**
 * @brief Simple struct to represent a screen coordinate (pixel).
 */
struct ScreenPoint {
  uint16_t x;
  uint16_t y;
};

/**
 * @brief Simple struct to represent a raw touch coordinate from the touch controller.
 */
struct TouchPoint {
  uint16_t xRaw;
  uint16_t yRaw;
};

/**
 * @brief Holds the calibration matrix used to convert raw touch values
 *        into screen coordinates.
 *
 * The transformation is of the form:
 *    x = αX * x′ + βX * y′ + ΔX
 *    y = αY * x′ + βY * y′ + ΔY
 *
 * These coefficients are calculated using a least-squares 5-point calibration method
 * described in the Texas Instruments paper:
 *
 * "Calibration in touch-screen systems"
 * by Tony Chang and Wendy Fang, Texas Instruments, 2007.
 * https://www.ti.com/lit/an/slyt277/slyt277.pdf
 */
struct CalibrationMatrix {
  float alphaX;
  float betaX;
  float deltaX;
  float alphaY;
  float betaY;
  float deltaY;
  bool valid = false;
};

/**
 * @brief Initializes and attempts to load the calibration matrix from EEPROM.
 *
 * @return true if valid calibration data was loaded.
 */
bool loadFromPreferences();

/**
 * @brief Saves the current calibration matrix to EEPROM for future use.
 */
void saveToPreferences();

/**
 * @brief Clears the calibration matrix (marks it invalid and optionally wipes EEPROM).
 */
void clearCalibration();

/**
 * @brief Checks if a valid calibration matrix is currently loaded.
 */
bool isCalibrated();

/**
 * @brief Computes a calibration matrix from at least 5 known screen/raw point pairs.
 *
 * This uses least squares matrix solving:
 *    (Aᵀ A)^-1 Aᵀ x
 *    (Aᵀ A)^-1 Aᵀ y
 *
 * @param screenPoints Array of known screen pixel points
 * @param touchPoints Array of raw touch points (same order)
 * @param count Number of points (must be >= 5)
 * @return true if computation succeeded
 */
bool computeFromSamples(const ScreenPoint* screenPoints, const TouchPoint* touchPoints, size_t count);

/**
 * @brief Applies the calibration matrix to raw touch coordinates.
 *
 * @param xRaw Raw x from touch controller
 * @param yRaw Raw y from touch controller
 * @param xCal Output: calibrated screen x
 * @param yCal Output: calibrated screen y
 * @return true if the matrix is valid and the mapping succeeded
 */
bool apply(uint16_t xRaw, uint16_t yRaw, uint16_t& xCal, uint16_t& yCal);

}  // namespace touch

#endif  // TOUCH_CALIBRATION_H_
