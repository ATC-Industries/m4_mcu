#include "SpeedModule.h"

#include <cmath>

#include "StateManager.h"
#include "lvgl.h"

// UI Elements (externs from UI event screen)
extern lv_obj_t *ui_SettingsTextareaCalibrationNumberTextArea;
extern lv_obj_t *ui_SettingsTextareaCalibrationCalculatorNumTeethTextArea;
extern lv_obj_t *ui_SettingsTextareaCalibrationCalculatorWheelDiameterTextArea;
extern lv_obj_t *ui_SettingsTextareaCalibrationCalculatorGearRatioTextArea;
extern lv_obj_t *ui_SettingsLabelGearToothCalculatorPulses;
extern lv_obj_t *ui_SettingsLabelAutoDriveCurrentPulses;

// Internal state
static int calibrationPulses = 1000;  // default/fallback
static int pulseCount = 0;
static bool driveOffMode = false;
static PullState currentPullState = PullState::READY;

static void IRAM_ATTR onSpeedSensorPulseISR() { SpeedModule::onPulseDetected(); }

void SpeedModule::begin() {
  pinMode(SPEED_SENSOR_PIN, INPUT_PULLUP);  // Assuming open-drain or contact-closure type sensor
  attachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_PIN), onSpeedSensorPulseISR, RISING);
}

// ---- Validation ----
bool SpeedModule::isValidCalibrationNumber(int pulses) {
  return pulses >= CALIBRATION_MIN && pulses <= CALIBRATION_MAX;
}

// ---- Manual Calibration ----
void SpeedModule::saveManualCalibration(int pulses) {
  if (!isValidCalibrationNumber(pulses)) return;
  calibrationPulses = pulses;
  StateManager::setSpeedCalibrationNumber(pulses);
}

// ---- Presets ----
void SpeedModule::applyRadarCalibration() {
  saveManualCalibration(RADAR_CALIBRATION_PULSES);
  lv_textarea_set_text(ui_SettingsTextareaCalibrationNumberTextArea, std::to_string(RADAR_CALIBRATION_PULSES).c_str());
}

void SpeedModule::applyGPSCalibration() {
  saveManualCalibration(GPS_CALIBRATION_PULSES);
  lv_textarea_set_text(ui_SettingsTextareaCalibrationNumberTextArea, std::to_string(GPS_CALIBRATION_PULSES).c_str());
}

// ---- Gear Tooth Calculation ----
int SpeedModule::calculateCalibrationFromInputs(int numTeeth, float wheelDiameterInches, float gearRatio) {
  float wheelCircumference = M_PI * wheelDiameterInches;
  float rotationsIn300ft = 300.0f / (wheelCircumference / 12.0f);  // Convert inches to feet
  int pulses = static_cast<int>(roundf(numTeeth * rotationsIn300ft * gearRatio));
  return pulses;
}

void SpeedModule::saveCalculatorCalibration() {
  int teeth = atoi(lv_textarea_get_text(ui_SettingsTextareaCalibrationCalculatorNumTeethTextArea));
  float diameter = atof(lv_textarea_get_text(ui_SettingsTextareaCalibrationCalculatorWheelDiameterTextArea));
  float ratio = atof(lv_textarea_get_text(ui_SettingsTextareaCalibrationCalculatorGearRatioTextArea));

  int pulses = calculateCalibrationFromInputs(teeth, diameter, ratio);
  if (!isValidCalibrationNumber(pulses)) {
    // Show error
    static const char *btn_txts[] = {NULL};

    // Help text
    const char *help_text = "Invalid calibration number. Must be between 250 and 25,000.";

    // Create a modal message box
    lv_obj_t *mbox = lv_msgbox_create(NULL, "INVALID CALIBRATION NUMBER", help_text, btn_txts, true);

    // Center the message box on the screen
    lv_obj_set_width(mbox, 300);  // Adjust the width here
    lv_obj_center(mbox);
    return;
  }

  lv_label_set_text_fmt(ui_SettingsLabelGearToothCalculatorPulses, "%d pulses", pulses);
  lv_textarea_set_text(ui_SettingsTextareaCalibrationNumberTextArea, std::to_string(pulses).c_str());
  saveManualCalibration(pulses);
}

// ---- Drive-Off Calibration ----
void SpeedModule::startDriveOffCalibration() {
  driveOffMode = true;
  pulseCount = 0;
  lv_label_set_text_fmt(ui_SettingsLabelAutoDriveCurrentPulses, "%d", pulseCount);
}

void SpeedModule::stopDriveOffCalibration() {
  driveOffMode = false;
  saveManualCalibration(pulseCount);
  lv_textarea_set_text(ui_SettingsTextareaCalibrationNumberTextArea, std::to_string(pulseCount).c_str());
}

bool SpeedModule::isDriveOffModeActive() { return driveOffMode; }

void SpeedModule::handlePulseDuringDriveOff() {
  if (!driveOffMode) return;
  pulseCount++;
  lv_label_set_text_fmt(ui_SettingsLabelAutoDriveCurrentPulses, "%d", pulseCount);
}

// ---- Runtime Tracking ----
void SpeedModule::onPulseDetected() {
  if (driveOffMode) {
    handlePulseDuringDriveOff();
    return;
  }

  if (currentPullState == PullState::PULLING) {
    pulseCount++;
    updateSpeedAndDistance();
  } else if (currentPullState == PullState::STAGED) {
    resetDistance();
  }
}

void SpeedModule::notifyPullStateChanged(PullState newState) {
  currentPullState = newState;
  if (newState == PullState::STAGED) resetDistance();
}

void SpeedModule::resetDistance() {
  pulseCount = 0;
  StateManager::setDistance(0.0f);
}

// Simple speed/distance update
void SpeedModule::updateSpeedAndDistance() {
  // TODO expand updateSpeedAndDistance() to compute real speed via pulse frequency and time deltas.
  float distanceFeet = (300.0f * static_cast<float>(pulseCount)) / static_cast<float>(calibrationPulses);
  float speed = 0.0f;  // You could integrate time-based delta here for true MPH later

  StateManager::setDistance(distanceFeet);
  StateManager::setSpeed(speed);
}

// ---- Optional Accessors ----
int SpeedModule::getCurrentPulseCount() { return pulseCount; }
float SpeedModule::getCurrentDistance() { return (300.0f * static_cast<float>(pulseCount)) / calibrationPulses; }
float SpeedModule::getCurrentSpeed() { return 0.0f; }  // Placeholder
