#include "SpeedModule.h"

#include <cmath>

#include "StateManager.h"
#include "lvgl.h"
#include "Logging.h"

#define SPEED_TIMEOUT_MICROS 1000000  // 1s

namespace {
  constexpr uint32_t kWarmupMs = 200;       // shorter
  constexpr int      kMinGoodIntervals = 2; // easier to satisfy
  constexpr float    kWarmupMinDistFt = 6.0f;
  constexpr float    kEmaAlpha = 0.25f;    // smoothing on mph

  uint32_t warmupUntilMs = 0;
  int      goodIntervals = 0;
  float    runningDeltaSec = 0.0f;        // running avg of period
  float    publishedMPH = 0.0f;           // smoothed

    // Tuning
  constexpr float kOutlierLow     = 0.7f;   // accept if >= 70% of running period
  constexpr float kOutlierHigh    = 1.5f;   // accept if <= 150% of running period
  constexpr int   kMaxRejects     = 6;      // after 6 rejects, force adapt
  constexpr float kMaxAccelMphS   = 60.0f;  // limit MPH change rate
  constexpr float kEMA            = 0.25f;  // speed smoothing

  static int      rejectStreak = 0;
  static uint32_t lastSpeedUpdateMs = 0;

}

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

void SpeedModule::startRun() {
  noInterrupts();
  isrIntervalSumMicros = 0;
  isrIntervalCount = 0;
  pendingPulses = 0;
  interrupts();

  warmupUntilMs = millis() + kWarmupMs;
  goodIntervals = 0;
  runningDeltaSec = 0.0f;
  publishedMPH = 0.0f;
}

bool SpeedModule::isWarmupActive() {
  bool timeOk = millis() < warmupUntilMs;
  bool intervalsOk = goodIntervals < kMinGoodIntervals;
  bool distOk = StateManager::getDistance() < kWarmupMinDistFt;
  return timeOk || intervalsOk || distOk;
}

void SpeedModule::tick() {
  const uint64_t now = micros();

  // snapshot ISR batch
  uint32_t pulses = 0, cnt = 0;
  uint64_t sumMicros = 0;
  if (pulseReceived) {
    noInterrupts();
    pulses = pendingPulses;           pendingPulses = 0;
    cnt    = isrIntervalCount;        isrIntervalCount = 0;
    sumMicros = isrIntervalSumMicros; isrIntervalSumMicros = 0;
    pulseReceived = false;
    interrupts();
  }

  if ((currentPullState == PullState::PULLING || currentPullState == PullState::STAGED) && pulses > 0) {
    pulseCount += (int)pulses;

    if (cnt > 0 && calibrationPulses > 0) {
      float avgDeltaSec = (float)sumMicros / (float)cnt / 1e6f;

      // bootstrap
if (runningDeltaSec <= 0.0f) runningDeltaSec = avgDeltaSec;

// Decide accept vs reject
bool inBand =
  (avgDeltaSec >= runningDeltaSec * kOutlierLow) &&
  (avgDeltaSec <= runningDeltaSec * kOutlierHigh);

if (inBand) {
  // normal adaptation
  runningDeltaSec = 0.7f * runningDeltaSec + 0.3f * avgDeltaSec;
  rejectStreak = 0;
} else {
  rejectStreak++;

  // If the sim is ramping faster than our band, slowly chase it
  // bias runningDeltaSec toward the new value a little so we do not stick forever
  runningDeltaSec = 0.9f * runningDeltaSec + 0.1f * avgDeltaSec;

  // If we have rejected too many in a row, force-accept this one
  if (rejectStreak >= kMaxRejects) {
    runningDeltaSec = 0.8f * runningDeltaSec + 0.2f * avgDeltaSec;
    rejectStreak = 0;
  }
}

// Convert to MPH using the adapted running period
const float feetPerPulse = 300.0f / calibrationPulses;
const float fps = feetPerPulse / runningDeltaSec;
float rawMPH = fps * 0.681818f;

// Rate limit huge jumps, then smooth
rawMPH = clampAccel(currentSpeedMPH, rawMPH);
publishedMPH = (1.0f - kEMA) * publishedMPH + kEMA * rawMPH;

// During warmup, you can still cap if you want, otherwise publish directly
// currentSpeedMPH = isWarmupActive() ? min(publishedMPH, 15.0f) : publishedMPH;
currentSpeedMPH = publishedMPH;


    }

    if (currentPullState == PullState::STAGED) resetDistance();
    updateSpeedAndDistance();
    return;
  }

  // decay on true timeout
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
    LOGD("[SpeedModule] Pulses: %d | Distance: %.2f ft | Speed: %.1f MPH\n",
                  pulseCount, distanceFeet, currentSpeedMPH);
  }
}

static float SpeedModule::clampAccel(float current, float target) {
  uint32_t nowMs = millis();
  float dt = (lastSpeedUpdateMs == 0) ? 0.01f : (nowMs - lastSpeedUpdateMs) / 1000.0f;
  float maxStep = kMaxAccelMphS * dt;
  if (target > current + maxStep) target = current + maxStep;
  if (target < current - maxStep) target = current - maxStep;
  lastSpeedUpdateMs = nowMs;
  return target;
}



// ---- Optional Accessors ----
int SpeedModule::getCurrentPulseCount() { return pulseCount; }
float SpeedModule::getCurrentDistance() { return (300.0f * static_cast<float>(pulseCount)) / calibrationPulses; }
float SpeedModule::getCurrentSpeed() { return 0.0f; }  // Placeholder