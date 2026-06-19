#include "StateManager.h"
#include "peripherals/PeripheralsInit.h"
#include <cstring>
#include <SpeedModule.h>

#define LOG_TAG "StateManager"
#include "Logging.h"

#include <Preferences.h>

static ::Preferences storage;
static bool s_preferencesDirty = false;
static unsigned long s_saveRequestedAtMs = 0;
static constexpr unsigned long kPreferencesSaveDebounceMs = 400;
static bool s_hasJudgeMirrorUnits = false;
static UnitSystem s_judgeMirrorUnitSystem = UnitSystem::IMPERIAL;
static float s_judgeMirrorTrackLengthFeet = 300.0f;

static void writePreferencesToStorage(const SystemPreferences& prefs) {
  storage.begin("m4prefs", false);

  storage.putUChar("unitSystem", static_cast<uint8_t>(prefs.unitSystem));
  storage.putString("className", prefs.pullingClassName);
  storage.putInt("classWeight", prefs.pullingClassWeight);
  storage.putString("driverName", prefs.driverName);
  storage.putInt("driverNumber", prefs.driverNumber);
  storage.putInt("M4ID", prefs.M4IDNumber);
  storage.putInt("HostM4ID", prefs.HostM4IDNumber);
  storage.putBool("isJudgeMode", prefs.isJudgeMode);

  storage.putBool("ls1_enabled", prefs.limitSwitchEnabled[0]);
  storage.putBool("ls2_enabled", prefs.limitSwitchEnabled[1]);

  for (int i = 0; i < 4; ++i) {
    storage.putBool(("relayEn" + String(i)).c_str(), prefs.relayEnabled[i]);
  }

  storage.putFloat("distAlarm1", prefs.distanceAlarm1);
  storage.putFloat("distAlarm2", prefs.distanceAlarm2);
  storage.putFloat("tachAlarm1", prefs.tachAlarm1);
  storage.putFloat("tachAlarm2", prefs.tachAlarm2);
  storage.putFloat("mphAlarm1", prefs.mphAlarm1);
  storage.putFloat("mphAlarm2", prefs.mphAlarm2);

  storage.putFloat("trackLength", prefs.trackLengthFeet);
  storage.putUChar("brightness", prefs.screenBrightness);
  storage.putBool("screenRot180", prefs.screenRotation180);
  storage.putBool("tachEnabled", prefs.tachEnabled);
  storage.putBool("limitsEnabled", prefs.limitSwitchesEnabled);
  storage.putBool("relaysEnabled", prefs.relaysEnabled);
  storage.putInt("speedCal", prefs.speedCalibrationPulses);

  storage.putBool("autoConnTrac", prefs.isAutoConnectTractor);
  storage.putBytes("tractorAddr", prefs.pairedTractorAddress, 6);
  storage.putBytes("remoteAddr", prefs.pairedRemoteDisplayAddress, 6);
  storage.putULong("pairingDelay", prefs.pairingDelay);

  storage.putInt("pullCount", prefs.pullHistoryCount);
  for (int i = 0; i < prefs.pullHistoryCount; i++) {
    String keyPrefix = "pull" + String(i) + "_";
    storage.putString((keyPrefix + "driver").c_str(), prefs.pullHistory[i].driverName);
    storage.putInt((keyPrefix + "drivNum").c_str(), prefs.pullHistory[i].driverNumber);
    storage.putString((keyPrefix + "class").c_str(), prefs.pullHistory[i].className);
    storage.putInt((keyPrefix + "weight").c_str(), prefs.pullHistory[i].classWeight);
    storage.putFloat((keyPrefix + "speed").c_str(), prefs.pullHistory[i].maxSpeedMPH);
    storage.putFloat((keyPrefix + "distance").c_str(), prefs.pullHistory[i].maxDistanceFeet);
    storage.putFloat((keyPrefix + "rpm").c_str(), prefs.pullHistory[i].maxRPM);
    storage.putULong((keyPrefix + "time").c_str(), prefs.pullHistory[i].timestamp);
  }

  storage.end();
}

SystemState StateManager::systemState;
SystemPreferences StateManager::preferences;

SystemState& StateManager::state() { return systemState; }
SystemPreferences& StateManager::prefs() { return preferences; }

// ----- Unit-aware getters -----

namespace {
UnitSystem getEffectiveUnitSystem() {
  if (StateManager::getJudgeMode() && s_hasJudgeMirrorUnits) {
    return s_judgeMirrorUnitSystem;
  }
  return StateManager::prefs().unitSystem;
}

float getEffectiveTrackLengthFeet() {
  if (StateManager::getJudgeMode() && s_hasJudgeMirrorUnits) {
    return s_judgeMirrorTrackLengthFeet;
  }
  return StateManager::prefs().trackLengthFeet;
}
}  // namespace

UnitSystem StateManager::getUnitSystem() { return getEffectiveUnitSystem(); }

float StateManager::getDistance() {
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.distanceInFeet
                                                            : systemState.distanceInFeet * 0.3048f;
}

float StateManager::getSpeed() {
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.speedInMPH
                                                            : systemState.speedInMPH * 1.60934f;
}

float StateManager::getTrackLength() {
  float trackLengthFeet = getEffectiveTrackLengthFeet();
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? trackLengthFeet
                                                            : trackLengthFeet * 0.3048f;
}

float StateManager::getRPM() { return systemState.rpm; }

float StateManager::getMaxRPM() { return systemState.maxRpm; }

float StateManager::getMaxSpeed() { 
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.maxSpeedInMPH
                                                            : systemState.maxSpeedInMPH * 1.60934f;
}

float StateManager::getMaxDistance() { 
      return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.maxDistanceInFeet
                                                                : systemState.maxDistanceInFeet * 0.3048f;
}

int StateManager::getM4ID() {
  return preferences.M4IDNumber;
} 

int StateManager::getHostM4ID() {
  return preferences.HostM4IDNumber;
} 

// ----- Setters -----

void StateManager::setUnitSystem(UnitSystem s) {
  LOGI("setUnitSystem -> %u", (unsigned)s);
  if (preferences.unitSystem == s) return;
  preferences.unitSystem = s;
  savePreferences();
}

void StateManager::setRPM(float rpm) {
  systemState.rpm = rpm;
  if (systemState.currentPullState == PullState::PULLING && rpm > systemState.maxRpm) {
    systemState.maxRpm = rpm;
  }
}

void StateManager::setSpeed(float mph) {
  systemState.speedInMPH = mph;
  if (systemState.currentPullState == PullState::PULLING && mph > systemState.maxSpeedInMPH) {
    systemState.maxSpeedInMPH = mph;
  }
}

void StateManager::setDistance(float ft) {
  systemState.distanceInFeet = ft;
  if (systemState.currentPullState == PullState::PULLING && ft > systemState.maxDistanceInFeet) {
    systemState.maxDistanceInFeet = ft;
  }
}

void StateManager::resetMaxRPM() { systemState.maxRpm = 0.0f; }

void StateManager::resetMaxSpeed() { systemState.maxSpeedInMPH = 0.0f; }

void StateManager::resetMaxDistance() { systemState.maxDistanceInFeet = 0.0f; }

void StateManager::resetAllMaxValues() {
  resetMaxRPM();
  resetMaxSpeed();
  resetMaxDistance();
}
void StateManager::setPullState(PullState newState) { systemState.currentPullState = newState; }

PullState StateManager::getPullState() { return systemState.currentPullState; }

int StateManager::getSpeedCalibrationNumber() { return preferences.speedCalibrationPulses; }

void StateManager::setSpeedCalibrationNumber(int pulses) {
  preferences.speedCalibrationPulses = pulses;
  savePreferences();
}

void StateManager::setScreenRotation(bool rotation180) {
  preferences.screenRotation180 = rotation180;
  
  savePreferences();
  LOGI("Screen rotation set to %s", rotation180 ? "180°" : "0°");
}

void StateManager::setM4ID(int unitId, bool persist) {
  int clamped = unitId;
  if (clamped < 0) {
    clamped = 0;
  } else if (clamped > 9999) {
    clamped = 9999;
  }

  bool changed = preferences.M4IDNumber != clamped;
  if (changed) {
    preferences.M4IDNumber = clamped;
  }

  if (persist && changed) {
    savePreferences();
  }
}

void StateManager::setHostM4ID(int unitId, bool persist) {
  int clamped = unitId;
  if (clamped < 0) {
    clamped = 0;
  } else if (clamped > 9999) {
    clamped = 9999;
  }

  bool changed = preferences.HostM4IDNumber != clamped;
  if (changed) {
    preferences.HostM4IDNumber = clamped;
  }

  if (persist && changed) {
    savePreferences();
  }
}

void StateManager::setJudgeMode(bool isJudgeMode, bool persist) {
  bool changed = preferences.isJudgeMode != isJudgeMode;
  if (changed) {
    preferences.isJudgeMode = isJudgeMode;
    systemState.judgeMode = isJudgeMode;  // sync runtime state
  }

  if (persist && changed) {
    savePreferences();
  }
}

void StateManager::setJudgeMirrorUnits(UnitSystem system, float trackLengthFeet) {
  s_judgeMirrorUnitSystem = system;
  if (trackLengthFeet > 0.0f) {
    s_judgeMirrorTrackLengthFeet = trackLengthFeet;
  }
  s_hasJudgeMirrorUnits = true;
}

void StateManager::clearJudgeMirrorUnits() {
  s_hasJudgeMirrorUnits = false;
}

bool StateManager::getJudgeMode() {
  return systemState.judgeMode;
}

bool StateManager::getJudgeModePreference() {
  return preferences.isJudgeMode;
}

void StateManager::setScreenBrightness(uint8_t level) {
  if (preferences.screenBrightness == level) return;
  preferences.screenBrightness = level;
  savePreferences();
}

void StateManager::setTrackLengthFeet(float feet) {
  if (feet <= 0.0f) return;                     // ignore bad values
  if (fabs(preferences.trackLengthFeet - feet) < 0.001f) return;
  preferences.trackLengthFeet = feet;
  savePreferences();
}

void StateManager::setTachEnabled(bool on) {
  if (preferences.tachEnabled == on) return;
  preferences.tachEnabled = on;
  savePreferences();
}

void StateManager::setLimitSwitchesEnabled(bool on) {
  if (preferences.limitSwitchesEnabled == on) return;
  preferences.limitSwitchesEnabled = on;
  savePreferences();
}

void StateManager::setRelaysEnabled(bool on) {
  if (preferences.relaysEnabled == on) return;
  preferences.relaysEnabled = on;
  savePreferences();
}


bool StateManager::getScreenRotation() {
  bool rotation180 = preferences.screenRotation180;
  LOGD("Current screen rotation is %s", rotation180 ? "180°" : "0°");
  return rotation180;
}



// This is a setting. this determines if the M4 will auto-connect to the Tach Tractor Should remain here in StateManager
void StateManager::setIsAutoConnectTractor(bool autoConnect) {
  preferences.isAutoConnectTractor = autoConnect;
  savePreferences();

  LOGI("Auto-connect to Tach Tractor set to %s", autoConnect ? "ENABLED" : "DISABLED");
}

bool StateManager::getIsAutoConnectTractor() {
  return preferences.isAutoConnectTractor;
}

void StateManager::setPairedTractorAddress(const uint8_t* addr) {
  if (!addr) return;
  memcpy(preferences.pairedTractorAddress, addr, 6);
  savePreferences();
}

const uint8_t* StateManager::getPairedTractorAddress() {
  return preferences.pairedTractorAddress;
}

void StateManager::savePullResult() {
  PullResult newPull;
  newPull.driverName = preferences.driverName;
  newPull.driverNumber = preferences.driverNumber;
  newPull.className = preferences.pullingClassName;
  newPull.classWeight = preferences.pullingClassWeight;
  newPull.maxSpeedMPH = systemState.maxSpeedInMPH;
  newPull.maxDistanceFeet = systemState.maxDistanceInFeet;
  newPull.maxRPM = systemState.maxRpm;
  newPull.timestamp = millis() / 1000;  // Convert to seconds

  // Shift existing pulls down if at capacity
  if (preferences.pullHistoryCount >= MAX_PULL_HISTORY) {
    for (int i = 1; i < MAX_PULL_HISTORY; i++) {
      preferences.pullHistory[i - 1] = preferences.pullHistory[i];
    }
    preferences.pullHistory[MAX_PULL_HISTORY - 1] = newPull;
  } else {
    // Add to end
    preferences.pullHistory[preferences.pullHistoryCount] = newPull;
    preferences.pullHistoryCount++;
  }

  savePreferences();
  LOGI("Pull saved: Driver: %s (#%d), Max Speed: %.2f MPH, Max Distance: %.2f ft, Max RPM: %.1f",
       newPull.driverName.c_str(), newPull.driverNumber, newPull.maxSpeedMPH, newPull.maxDistanceFeet, newPull.maxRPM);
}

int StateManager::getPullHistoryCount() {
  return preferences.pullHistoryCount;
}

const PullResult* StateManager::getPullHistory() {
  return preferences.pullHistory;
}

const PullResult* StateManager::getPullResult(int index) {
  if (index < 0 || index >= preferences.pullHistoryCount) {
    return nullptr;
  }
  return &preferences.pullHistory[index];
}

void StateManager::clearPullHistory() {
  // Reset pull history count to 0
  preferences.pullHistoryCount = 0;
  
  // Reset driver number to 1
  preferences.driverNumber = 1;
  
  // Clear all pull history entries (optional, but good practice)
  for (int i = 0; i < MAX_PULL_HISTORY; i++) {
    preferences.pullHistory[i] = PullResult{};
  }
  
  // Save preferences to persist changes
  savePreferences();
  flushPreferencesNow();
  
  LOGI("Pull history cleared and driver number reset to 1");
}

RelayState StateManager::getRelayState(int index) {
  if (index < 0 || index >= 4) return RelayState::DISENGAGED;
  return systemState.relayStates[index];
}

void StateManager::setRelayState(int index, RelayState state) {
  if (index < 0 || index >= 4) return;
  systemState.relayStates[index] = state;
}

bool StateManager::getLimitSwitchTriggered(int index) {
  if (index < 0 || index >= 2) return false;
  return systemState.limitSwitchTriggered[index];
}

void StateManager::setLimitSwitchTriggered(int index, bool triggered) {
  if (index < 0 || index >= 2) return;
  systemState.limitSwitchTriggered[index] = triggered;
}

bool StateManager::isLimitSwitchEnabled(int index) {
  if (index < 0 || index >= 2) return false;
  return preferences.limitSwitchEnabled[index];
}

void StateManager::setLimitSwitchEnabled(int index, bool enabled) {
  if (index < 0 || index >= 2) return;
  preferences.limitSwitchEnabled[index] = enabled;
}

// ----- Preferences -----

void StateManager::loadPreferences() {
  PullState ps = getPullState();
  if (ps == PullState::PULLING || SpeedModule::isDriveOffModeActive()) return;
  storage.begin("m4prefs", false);

  preferences.unitSystem =
      static_cast<UnitSystem>(storage.getUChar("unitSystem", static_cast<uint8_t>(UnitSystem::IMPERIAL)));

  preferences.pullingClassName = storage.getString("className", "M4 Sled Monitor - " + String(DEVICE_VERSION));
  preferences.pullingClassWeight = storage.getInt("classWeight", 0);
  preferences.driverName = storage.getString("driverName", "Driver");
  preferences.driverNumber = storage.getInt("driverNumber", 1);

  preferences.M4IDNumber = storage.getInt("M4ID", 0);
  preferences.HostM4IDNumber = storage.getInt("HostM4ID", 0);
  preferences.isJudgeMode = storage.getBool("isJudgeMode", false);
  systemState.judgeMode = preferences.isJudgeMode;


  preferences.limitSwitchEnabled[0] = storage.getBool("ls1_enabled", true);
  preferences.limitSwitchEnabled[1] = storage.getBool("ls2_enabled", true);

  for (int i = 0; i < 4; ++i) {
    preferences.relayEnabled[i] = storage.getBool(("relayEn" + String(i)).c_str(), true);
  }

  preferences.distanceAlarm1 = storage.getFloat("distAlarm1", 0.0f);
  preferences.distanceAlarm2 = storage.getFloat("distAlarm2", 0.0f);
  preferences.tachAlarm1 = storage.getFloat("tachAlarm1", 0.0f);
  preferences.tachAlarm2 = storage.getFloat("tachAlarm2", 0.0f);
  preferences.mphAlarm1 = storage.getFloat("mphAlarm1", 0.0f);
  preferences.mphAlarm2 = storage.getFloat("mphAlarm2", 0.0f);

  preferences.trackLengthFeet = storage.getFloat("trackLength", 300.0f);

  preferences.screenBrightness = storage.getUChar("brightness", 100);
  preferences.screenRotation180 = storage.getBool("screenRot180", false);
  preferences.tachEnabled = storage.getBool("tachEnabled", true);
  preferences.limitSwitchesEnabled = storage.getBool("limitsEnabled", true);
  preferences.relaysEnabled = storage.getBool("relaysEnabled", true);

  preferences.isAutoConnectTractor = storage.getBool("autoConnTrac", true);

  preferences.speedCalibrationPulses = storage.getInt("speedCal", 1000);

  // Load M4 communication settings
  storage.getBytes("tractorAddr", preferences.pairedTractorAddress, 6);
  storage.getBytes("remoteAddr", preferences.pairedRemoteDisplayAddress, 6);
  preferences.pairingDelay = storage.getULong("pairingDelay", 10000);

  // Load pull history
  preferences.pullHistoryCount = storage.getInt("pullCount", 0);
  if (preferences.pullHistoryCount > MAX_PULL_HISTORY) {
    preferences.pullHistoryCount = MAX_PULL_HISTORY;
  }
  
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    String keyPrefix = "pull" + String(i) + "_";
    preferences.pullHistory[i].driverName = storage.getString((keyPrefix + "driver").c_str(), "");
    preferences.pullHistory[i].driverNumber = storage.getInt((keyPrefix + "drivNum").c_str(), 0);
    preferences.pullHistory[i].className = storage.getString((keyPrefix + "class").c_str(), "");
    preferences.pullHistory[i].classWeight = storage.getInt((keyPrefix + "weight").c_str(), 0);
    preferences.pullHistory[i].maxSpeedMPH = storage.getFloat((keyPrefix + "speed").c_str(), 0.0f);
    preferences.pullHistory[i].maxDistanceFeet = storage.getFloat((keyPrefix + "distance").c_str(), 0.0f);
    preferences.pullHistory[i].maxRPM = storage.getFloat((keyPrefix + "rpm").c_str(), 0.0f);
    preferences.pullHistory[i].timestamp = storage.getULong((keyPrefix + "time").c_str(), 0);
  }

  storage.end();
  LOGD("LimitSwitchEnabled: LS1 = %d, LS2 = %d", preferences.limitSwitchEnabled[0],
       preferences.limitSwitchEnabled[1]);

  // Print pull history
  LOGD("Pull History (%d pulls):", preferences.pullHistoryCount);
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    const PullResult& pull = preferences.pullHistory[i];
    LOGD("%d: Driver %s (#%d), Speed: %.2f MPH, Distance: %.2f ft, RPM: %.1f, Time: %lu",
         i, pull.driverName.c_str(), pull.driverNumber, pull.maxSpeedMPH,
         pull.maxDistanceFeet, pull.maxRPM, pull.timestamp);
  }

  LOGI("Preferences loaded successfully.");
}

void StateManager::savePreferences() {
  s_preferencesDirty = true;
  s_saveRequestedAtMs = millis();
}

void StateManager::flushPreferencesNow() {
  if (!s_preferencesDirty) return;

  PullState ps = getPullState();
  if (ps == PullState::PULLING || SpeedModule::isDriveOffModeActive()) return;

  writePreferencesToStorage(preferences);
  s_preferencesDirty = false;
}

void StateManager::processPendingSave() {
  if (!s_preferencesDirty) return;
  if (millis() - s_saveRequestedAtMs < kPreferencesSaveDebounceMs) return;
  flushPreferencesNow();
}
