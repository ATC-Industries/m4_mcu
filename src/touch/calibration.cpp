/**
 * @file calibration.cpp
 * @brief Implements touchscreen calibration matrix logic for mapping raw touch coordinates
 *        to screen coordinates using a least-squares algorithm.
 *
 * This implementation is based on the method described in the Texas Instruments paper:
 * "Calibration in touch-screen systems" by Tony Chang and Wendy Fang.
 * Link: https://www.ti.com/lit/pdf/slyt277
 *
 * The algorithm computes a linear transformation matrix using five or more sample points,
 * solving for six coefficients (αX, βX, ΔX, αY, βY, ΔY) via least-squares estimation.
 */

#include "touch/calibration.h"

#include <Arduino.h>
#include <Preferences.h>

#include "Config.h"

// Preferences keys
namespace {
constexpr const char* PREFS_NAMESPACE = "touch";
constexpr const char* PREFS_KEY_MATRIX = "matrix";

// Internal storage for current calibration matrix
touch::CalibrationMatrix g_matrix;
}  // namespace

namespace touch {

bool isCalibrated() { return g_matrix.valid; }

void clearCalibration() {
  g_matrix = {};
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.remove(PREFS_KEY_MATRIX);
  prefs.end();
  Serial.println("Calibration cleared.");
}

bool loadFromPreferences() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, true);  // read-only
  size_t expectedSize = sizeof(CalibrationMatrix);

  if (prefs.isKey(PREFS_KEY_MATRIX)) {
    size_t len = prefs.getBytesLength(PREFS_KEY_MATRIX);
    if (len == expectedSize) {
      prefs.getBytes(PREFS_KEY_MATRIX, &g_matrix, expectedSize);
      g_matrix.valid = true;
      prefs.end();
      Serial.println("Calibration matrix loaded from EEPROM.");
      return true;
    } else {
      Serial.println("Calibration data found but size mismatch. Ignoring.");
    }
  } else {
    Serial.println("No calibration data found.");
  }

  prefs.end();
  g_matrix.valid = false;
  return false;
}

void saveToPreferences() {
  if (!g_matrix.valid) {
    Serial.println("Attempted to save invalid calibration matrix.");
    return;
  }

  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putBytes(PREFS_KEY_MATRIX, &g_matrix, sizeof(CalibrationMatrix));
  prefs.end();

  Serial.println("Calibration matrix saved to EEPROM.");
}

bool apply(uint16_t xRaw, uint16_t yRaw, uint16_t& xCal, uint16_t& yCal) {
  if (!g_matrix.valid) return false;

  float fx = g_matrix.alphaX * xRaw + g_matrix.betaX * yRaw + g_matrix.deltaX;
  float fy = g_matrix.alphaY * xRaw + g_matrix.betaY * yRaw + g_matrix.deltaY;

  xCal = static_cast<uint16_t>(roundf(fx));
  yCal = static_cast<uint16_t>(roundf(fy));
  return true;
}

bool computeFromSamples(const ScreenPoint* screenPoints, const TouchPoint* touchPoints, size_t count) {
  if (count < 5) {
    Serial.println("Error: At least 5 calibration points required.");
    return false;
  }

  // Prepare least squares matrix A and vectors Bx and By
  // A: [ x′ y′ 1 ]
  float sumX1 = 0, sumY1 = 0, sum1 = 0;
  float sumX2 = 0, sumXY = 0, sumY2 = 0;
  float sumX_Screen = 0, sumY_Screen = 0;
  float sumXX_Screen = 0, sumXY_Screen = 0, sumYX_Screen = 0, sumYY_Screen = 0;

  for (size_t i = 0; i < count; ++i) {
    float x = static_cast<float>(touchPoints[i].xRaw);
    float y = static_cast<float>(touchPoints[i].yRaw);
    float sx = static_cast<float>(screenPoints[i].x);
    float sy = static_cast<float>(screenPoints[i].y);

    sumX1 += x;
    sumY1 += y;
    sum1 += 1.0f;

    sumX2 += x * x;
    sumY2 += y * y;
    sumXY += x * y;

    sumX_Screen += sx;
    sumXX_Screen += x * sx;
    sumXY_Screen += y * sx;

    sumY_Screen += sy;
    sumYX_Screen += x * sy;
    sumYY_Screen += y * sy;
  }

  // Construct normal matrix ATA and inverse it manually (3x3)
  float ATA[3][3] = {{sumX2, sumXY, sumX1}, {sumXY, sumY2, sumY1}, {sumX1, sumY1, sum1}};

  float det = ATA[0][0] * (ATA[1][1] * ATA[2][2] - ATA[2][1] * ATA[1][2]) -
              ATA[0][1] * (ATA[1][0] * ATA[2][2] - ATA[2][0] * ATA[1][2]) +
              ATA[0][2] * (ATA[1][0] * ATA[2][1] - ATA[2][0] * ATA[1][1]);

  if (fabsf(det) < 1e-6f) {
    Serial.println("Matrix inversion failed: determinant is too small.");
    return false;
  }

  float invDet = 1.0f / det;

  float ATA_inv[3][3];
  ATA_inv[0][0] = (ATA[1][1] * ATA[2][2] - ATA[2][1] * ATA[1][2]) * invDet;
  ATA_inv[0][1] = -(ATA[0][1] * ATA[2][2] - ATA[2][1] * ATA[0][2]) * invDet;
  ATA_inv[0][2] = (ATA[0][1] * ATA[1][2] - ATA[1][1] * ATA[0][2]) * invDet;

  ATA_inv[1][0] = -(ATA[1][0] * ATA[2][2] - ATA[2][0] * ATA[1][2]) * invDet;
  ATA_inv[1][1] = (ATA[0][0] * ATA[2][2] - ATA[2][0] * ATA[0][2]) * invDet;
  ATA_inv[1][2] = -(ATA[0][0] * ATA[1][2] - ATA[1][0] * ATA[0][2]) * invDet;

  ATA_inv[2][0] = (ATA[1][0] * ATA[2][1] - ATA[2][0] * ATA[1][1]) * invDet;
  ATA_inv[2][1] = -(ATA[0][0] * ATA[2][1] - ATA[2][0] * ATA[0][1]) * invDet;
  ATA_inv[2][2] = (ATA[0][0] * ATA[1][1] - ATA[1][0] * ATA[0][1]) * invDet;

  // Solve: [α β Δ] = ATA_inv * ATb
  g_matrix.alphaX = ATA_inv[0][0] * sumXX_Screen + ATA_inv[0][1] * sumXY_Screen + ATA_inv[0][2] * sumX_Screen;
  g_matrix.betaX = ATA_inv[1][0] * sumXX_Screen + ATA_inv[1][1] * sumXY_Screen + ATA_inv[1][2] * sumX_Screen;
  g_matrix.deltaX = ATA_inv[2][0] * sumXX_Screen + ATA_inv[2][1] * sumXY_Screen + ATA_inv[2][2] * sumX_Screen;

  g_matrix.alphaY = ATA_inv[0][0] * sumYX_Screen + ATA_inv[0][1] * sumYY_Screen + ATA_inv[0][2] * sumY_Screen;
  g_matrix.betaY = ATA_inv[1][0] * sumYX_Screen + ATA_inv[1][1] * sumYY_Screen + ATA_inv[1][2] * sumY_Screen;
  g_matrix.deltaY = ATA_inv[2][0] * sumYX_Screen + ATA_inv[2][1] * sumYY_Screen + ATA_inv[2][2] * sumY_Screen;

  g_matrix.valid = true;

  Serial.println("Calibration matrix computed successfully.");
  return true;
}

}  // namespace touch
