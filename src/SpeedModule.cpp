#include "SpeedModule.h"

#include <cmath>

#include "StateManager.h"
#include "lvgl.h"
#include "Logging.h"

#define SPEED_TIMEOUT_MICROS 1000000  // 1s

static unsigned long lastPulseMicros = 0;
static float currentSpeedMPH = 0.0f;

static const int SPEED_BUFFER_SIZE = 10;
static float deltaSecHistory[SPEED_BUFFER_SIZE];
static volatile int timingIndex = 0;
static volatile bool bufferFull = false;

// timing
static volatile uint32_t pendingPulses = 0;
static volatile bool     pulseReceived = false;

static volatile uint64_t isrLastMicros = 0;           // last pulse time (from ISR)
static volatile uint64_t isrIntervalSumMicros = 0;    // sum of intervals since last tick
static volatile uint32_t isrIntervalCount = 0;        // intervals counted since last tick


// UI Elements (externs from UI event screen)
extern lv_obj_t *ui_Settings1TextareaCalibrationNumberTextArea;
extern lv_obj_t *ui_Settings1TextareaCalibrationCalculatorNumTeethTextArea;
extern lv_obj_t *ui_Settings1TextareaCalibrationCalculatorWheelDiameterTextArea;
extern lv_obj_t *ui_Settings1TextareaCalibrationCalculatorGearRatioTextArea;
extern lv_obj_t *ui_Settings1LabelGearToothCalculatorPulses;
extern lv_obj_t *ui_Settings1LabelAutoDriveCurrentPulses;

// Internal state
static int calibrationPulses = 1000;  // default/fallback
static int pulseCount = 0;
static bool driveOffMode = false;
static PullState currentPullState = PullState::READY;

static void IRAM_ATTR onSpeedSensorPulseISR() {
  uint64_t now = micros();

  if (isrLastMicros != 0) {
    uint32_t delta = (uint32_t)(now - isrLastMicros);
    // Optional sanity clamp to ignore freak spikes
    if (delta >= 100 && delta <= 2000000) { // 100 µs to 2 s
      isrIntervalSumMicros += delta;
      isrIntervalCount++;
    }
  }
  isrLastMicros = now;

  pendingPulses++;
  pulseReceived = true;
}


void SpeedModule::begin() {
  pinMode(SPEED_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SPEED_SENSOR_PIN), onSpeedSensorPulseISR, RISING);

  timingIndex = 0;
  bufferFull = false;
  lastPulseMicros = 0;
  pulseCount = 0;
  currentSpeedMPH = 0.0f;

  isrLastMicros = 0;
  isrIntervalSumMicros = 0;
  isrIntervalCount = 0;

  for (int i = 0; i < SPEED_BUFFER_SIZE; i++) deltaSecHistory[i] = 0.0f;
}

void SpeedModule::tick() {
  uint64_t now = micros();
  // Snapshot ISR data
  uint32_t pulses = 0;
  uint64_t sumMicros = 0;
  uint32_t cnt = 0;

  if (pulseReceived) {
    noInterrupts();
    pulses = pendingPulses;           pendingPulses = 0;
    sumMicros = isrIntervalSumMicros; isrIntervalSumMicros = 0;
    cnt = isrIntervalCount;           isrIntervalCount = 0;
    pulseReceived = false;
    interrupts();
  }

  if (driveOffMode) {
    if (pulses) {
      pulseCount += pulses;
      lv_label_set_text_fmt(ui_Settings1LabelAutoDriveCurrentPulses, "%d", pulseCount);
    }
    return;
  }

  if ((currentPullState == PullState::PULLING || currentPullState == PullState::STAGED) && pulses > 0) {
    pulseCount += (int)pulses;

    // Average period from ISR
    if (cnt > 0 && calibrationPulses > 0) {
      const float avgDeltaSec = (float)sumMicros / (float)cnt / 1e6f;
      if (avgDeltaSec > 0.0001f && avgDeltaSec < 2.0f) {
        // optional: smooth with your small ring buffer
        deltaSecHistory[timingIndex] = avgDeltaSec;
        timingIndex = (timingIndex + 1) % SPEED_BUFFER_SIZE;
        if (timingIndex == 0) bufferFull = true;

        const float avg = getAverageDeltaSec();
        const float feetPerPulse = 300.0f / calibrationPulses;
        const float fps = feetPerPulse / avg;
        currentSpeedMPH = fps * 0.681818f;
      }
    }

    if (currentPullState == PullState::STAGED) resetDistance();
    updateSpeedAndDistance();
    return;
  }

  // Only decay to zero if we truly have not seen pulses in a while
  if (isrLastMicros > 0 && (now - isrLastMicros > SPEED_TIMEOUT_MICROS)) {
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
  lv_textarea_set_text(ui_Settings1TextareaCalibrationNumberTextArea, std::to_string(RADAR_CALIBRATION_PULSES).c_str());
}

void SpeedModule::applyGPSCalibration() {
  saveManualCalibration(GPS_CALIBRATION_PULSES);
  lv_textarea_set_text(ui_Settings1TextareaCalibrationNumberTextArea, std::to_string(GPS_CALIBRATION_PULSES).c_str());
}

// ---- Gear Tooth Calculation ----
int SpeedModule::calculateCalibrationFromInputs(int numTeeth, float wheelDiameterInches, float gearRatio) {
  float wheelCircumference = M_PI * wheelDiameterInches;
  float rotationsIn300ft = 300.0f / (wheelCircumference / 12.0f);  // Convert inches to feet
  int pulses = static_cast<int>(roundf(numTeeth * rotationsIn300ft * gearRatio));
  return pulses;
}

void SpeedModule::saveCalculatorCalibration() {
  int teeth = atoi(lv_textarea_get_text(ui_Settings1TextareaCalibrationCalculatorNumTeethTextArea));
  float diameter = atof(lv_textarea_get_text(ui_Settings1TextareaCalibrationCalculatorWheelDiameterTextArea));
  float ratio = atof(lv_textarea_get_text(ui_Settings1TextareaCalibrationCalculatorGearRatioTextArea));

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

  lv_label_set_text_fmt(ui_Settings1LabelGearToothCalculatorPulses, "%d pulses", pulses);
  lv_textarea_set_text(ui_Settings1TextareaCalibrationNumberTextArea, std::to_string(pulses).c_str());
  saveManualCalibration(pulses);
}

// ---- Drive-Off Calibration ----
void SpeedModule::startDriveOffCalibration() {
  driveOffMode = true;
  pulseCount = 0;
  lv_label_set_text_fmt(ui_Settings1LabelAutoDriveCurrentPulses, "%d", pulseCount);
}

void SpeedModule::stopDriveOffCalibration() {
  driveOffMode = false;
  saveManualCalibration(pulseCount);
  lv_textarea_set_text(ui_Settings1TextareaCalibrationNumberTextArea, std::to_string(pulseCount).c_str());
}

bool SpeedModule::isDriveOffModeActive() { return driveOffMode; }

void SpeedModule::handlePulseDuringDriveOff() {
  if (!driveOffMode) return;
  pulseCount++;
  lv_label_set_text_fmt(ui_Settings1LabelAutoDriveCurrentPulses, "%d", pulseCount);
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
  static uint32_t lastLogMs = 0;
  const float distanceFeet = (300.0f * (float)pulseCount) / calibrationPulses;

  // push to state
  StateManager::setSpeed(currentSpeedMPH);
  StateManager::setDistance(distanceFeet);

  // log at ~10 Hz max
  uint32_t ms = millis();
  if (ms - lastLogMs >= 100) {
    lastLogMs = ms;
    LOGD("[SpeedModule] Pulses: %d | Distance: %.2f ft | Speed: %.2f MPH\n",
                  pulseCount, distanceFeet, currentSpeedMPH);
  }
}


// ---- Optional Accessors ----
int SpeedModule::getCurrentPulseCount() { return pulseCount; }
float SpeedModule::getCurrentDistance() { return (300.0f * static_cast<float>(pulseCount)) / calibrationPulses; }
float SpeedModule::getCurrentSpeed() { return 0.0f; }  // Placeholder