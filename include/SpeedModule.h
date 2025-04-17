#ifndef SPEED_MODULE_H
#define SPEED_MODULE_H

#include "Config.h"            // For radar/gps cal values
#include "PullStateManager.h"  // For pull state enum

namespace SpeedModule {

void begin();  // Call this once during setup()
void tick();   // Call this every loop

// ---- Calibration ----
bool isValidCalibrationNumber(int pulses);
void saveManualCalibration(int pulses);
void applyRadarCalibration();
void applyGPSCalibration();
int calculateCalibrationFromInputs(int numTeeth, float wheelDiameterInches, float gearRatio);
void saveCalculatorCalibration();

// ---- Drive-Off Calibration ----
void startDriveOffCalibration();
void stopDriveOffCalibration();
bool isDriveOffModeActive();
void handlePulseDuringDriveOff();

// ---- Runtime Tracking ----
void onPulseDetected();
void notifyPullStateChanged(PullState newState);
void resetDistance();
void updateSpeedAndDistance();

// ---- Internals (optional: expose for debugging/testing) ----
int getCurrentPulseCount();
float getCurrentSpeed();
float getCurrentDistance();
}  // namespace SpeedModule

#endif  // SPEED_MODULE_H
