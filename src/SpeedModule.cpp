#include "SpeedModule.h"

#include <cmath>

#include "StateManager.h"
#include "lvgl.h"

#define SPEED_TIMEOUT_MICROS 500000  // 500ms

static unsigned long lastPulseMicros = 0;
static float currentSpeedMPH = 0.0f;
static volatile int pendingPulses = 0;
static volatile bool pulseReceived = false;

static const int SPEED_BUFFER_SIZE = 10;
static float deltaSecHistory[SPEED_BUFFER_SIZE];
static volatile int timingIndex = 0;
static volatile bool bufferFull = false;

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

static void IRAM_ATTR onSpeedSensorPulseISR() {
  pendingPulses++;
  pulseReceived = true;
}

void SpeedModule::begin() {
  pinMode(SPEED_SENSOR_PIN, INPUT_PULLUP);  // Assuming open-drain or contact-closure type sensor
  attachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_PIN), onSpeedSensorPulseISR, RISING);

  timingIndex = 0;
  bufferFull = false;
  lastPulseMicros = 0;
  pulseCount = 0;
  currentSpeedMPH = 0.0f;

  for (int i = 0; i < SPEED_BUFFER_SIZE; i++) {
    deltaSecHistory[i] = 0.0f;
  }
}

// void SpeedModule::tick() {
//   unsigned long now = micros();

//   if (driveOffMode) {
//     handlePulseDuringDriveOff();
//     return;
//   }

//   // Handle pulse updates
//   if (pulseReceived) {
//     noInterrupts();
//     int pulses = pendingPulses;
//     pendingPulses = 0;
//     pulseReceived = false;
//     interrupts();

//     if (currentPullState == PullState::PULLING || currentPullState == PullState::STAGED) {
//       if (pulses > 0) {
//         pulseCount += pulses;

//         if (lastPulseMicros > 0 && pulses == 1) {
//           float deltaSec = (now - lastPulseMicros) / 1e6f;
//           if (deltaSec > 0.0f && calibrationPulses > 0) {
//             float feetPerPulse = 300.0f / calibrationPulses;
//             float feetPerSecond = feetPerPulse / deltaSec;
//             currentSpeedMPH = feetPerSecond * 0.681818f;
//           }
//         }

//         lastPulseMicros = now;
//       }

//       updateSpeedAndDistance();
//       if (currentPullState == PullState::STAGED) {
//         resetDistance();
//         //lastPulseMicros = 0;
//       }
//     }
//     return;
//   }

//   // Handle speed decay if no new pulse
//   if (lastPulseMicros > 0 && (now - lastPulseMicros > SPEED_TIMEOUT_MICROS)) {
//     if (currentSpeedMPH > 0.01f) {
//       currentSpeedMPH = 0.0f;
//       updateSpeedAndDistance();
//     }
//   }
// }
void SpeedModule::tick() {
  unsigned long now = micros();



  if (pulseReceived) {
    noInterrupts();
    int pulses = pendingPulses;
    pendingPulses = 0;
    pulseReceived = false;
    interrupts();

    if (driveOffMode) {
      handlePulseDuringDriveOff();
      return;
    }

    if ((currentPullState == PullState::PULLING || currentPullState == PullState::STAGED) && pulses > 0) {
      pulseCount += pulses;

      for (int i = 0; i < pulses; i++) {
        unsigned long thisPulseMicros = now;  // could add per-pulse micro timing here if needed
        if (lastPulseMicros > 0) {
          float deltaSec = (thisPulseMicros - lastPulseMicros) / 1e6f;

          // Skip obviously bad values
          if (deltaSec >= 0.001f && deltaSec <= 2.0f && calibrationPulses > 0) {
            deltaSecHistory[timingIndex] = deltaSec;
            timingIndex = (timingIndex + 1) % SPEED_BUFFER_SIZE;
            if (timingIndex == 0) bufferFull = true;

            float avgDelta = getAverageDeltaSec();
            float feetPerPulse = 300.0f / calibrationPulses;
            currentSpeedMPH = (feetPerPulse / avgDelta) * 0.681818f;
          }
        }

        lastPulseMicros = thisPulseMicros;
      }

      updateSpeedAndDistance();
      if (currentPullState == PullState::STAGED) {
        resetDistance();
      }
    }
    return;
  }

  // Speed decay
  if (lastPulseMicros > 0 && (now - lastPulseMicros > SPEED_TIMEOUT_MICROS)) {
    if (currentSpeedMPH > 0.01f) {
      currentSpeedMPH = 0.0f;
      updateSpeedAndDistance();
    }
  }
}

float SpeedModule::getAverageDeltaSec() {
  int count = bufferFull ? SPEED_BUFFER_SIZE : timingIndex;
  if (count == 0) return 1.0f;  // prevent div/0 on boot

  float sum = 0.0f;
  for (int i = 0; i < count; i++) {
    sum += deltaSecHistory[i];
  }
  return sum / count;
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
  unsigned long now = micros();

  if (driveOffMode) {
    handlePulseDuringDriveOff();
    return;
  }

  if (currentPullState == PullState::PULLING) {
    pulseCount++;

    if (lastPulseMicros != 0) {
      unsigned long deltaMicros = now - lastPulseMicros;
      if (deltaMicros > 0) {
        float timeSeconds = deltaMicros / 1e6f;

        // Distance per pulse in feet
        float feetPerPulse = 300.0f / static_cast<float>(calibrationPulses);
        float feetPerSecond = feetPerPulse / timeSeconds;
        currentSpeedMPH = feetPerSecond * 0.681818f;  // 1 fps = 0.681818 mph
      }
    }

    lastPulseMicros = now;

    updateSpeedAndDistance();
  } else if (currentPullState == PullState::STAGED) {
    resetDistance();
    lastPulseMicros = 0;
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

void SpeedModule::updateSpeedAndDistance() {
  float distanceFeet = (300.0f * static_cast<float>(pulseCount)) / calibrationPulses;

  Serial.printf("[SpeedModule] Pulses: %d | Distance: %.2f ft | Speed: %.2f MPH\n", pulseCount, distanceFeet,
                currentSpeedMPH);

  StateManager::setSpeed(currentSpeedMPH);  // Always track live speed
  StateManager::setDistance(distanceFeet);  // Always track live distance
}

// ---- Optional Accessors ----
int SpeedModule::getCurrentPulseCount() { return pulseCount; }
float SpeedModule::getCurrentDistance() { return (300.0f * static_cast<float>(pulseCount)) / calibrationPulses; }
float SpeedModule::getCurrentSpeed() { return 0.0f; }  // Placeholder