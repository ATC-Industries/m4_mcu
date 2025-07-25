#include "StateManager.h"

#include <Preferences.h>

static ::Preferences storage;

SystemState StateManager::systemState;
SystemPreferences StateManager::preferences;

SystemState& StateManager::state() { return systemState; }
SystemPreferences& StateManager::prefs() { return preferences; }

// ----- Unit-aware getters -----

UnitSystem StateManager::getUnitSystem() { return preferences.unitSystem; }

float StateManager::getDistance() {
  return (preferences.unitSystem == UnitSystem::IMPERIAL) ? systemState.distanceInFeet
                                                          : systemState.distanceInFeet * 0.3048f;
}

float StateManager::getSpeed() {
  return (preferences.unitSystem == UnitSystem::IMPERIAL) ? systemState.speedInMPH : systemState.speedInMPH * 1.60934f;
}

float StateManager::getTrackLength() {
  return (preferences.unitSystem == UnitSystem::IMPERIAL) ? preferences.trackLengthFeet
                                                          : preferences.trackLengthFeet * 0.3048f;
}

float StateManager::getRPM() { return systemState.rpm; }

float StateManager::getMaxRPM() { return systemState.maxRpm; }

float StateManager::getMaxSpeed() { return systemState.maxSpeedInMPH; }

float StateManager::getMaxDistance() { return systemState.maxDistanceInFeet; }

// ----- Setters -----

void StateManager::setUnitSystem(UnitSystem system) {
  preferences.unitSystem = system;
  savePreferences();  // Save preferences when unit system changes
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
  Serial.printf("[StateManager] Pull saved: Driver: %s (#%d), Max Speed: %.2f MPH, Max Distance: %.2f ft, Max RPM: %.1f\n", 
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
  
  Serial.println("[StateManager] Pull history cleared and driver number reset to 1");
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
  storage.begin("m4prefs", false);

  preferences.unitSystem =
      static_cast<UnitSystem>(storage.getUChar("unitSystem", static_cast<uint8_t>(UnitSystem::IMPERIAL)));

  preferences.pullingClassName = storage.getString("className", "M4 Sled Monitor - " + String(VERSION));
  preferences.pullingClassWeight = storage.getInt("classWeight", 0);
  preferences.driverName = storage.getString("driverName", "Driver");
  preferences.driverNumber = storage.getInt("driverNumber", 1);

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

  preferences.benchmarkMode = storage.getBool("benchmark", false);
  preferences.screenBrightness = storage.getUChar("brightness", 100);
  preferences.tachEnabled = storage.getBool("tachEnabled", true);
  preferences.limitSwitchesEnabled = storage.getBool("limitsEnabled", true);
  preferences.relaysEnabled = storage.getBool("relaysEnabled", true);

  preferences.speedCalibrationPulses = storage.getInt("speedCal", 1000);

  // Load pull history
  preferences.pullHistoryCount = storage.getInt("pullCount", 0);
  if (preferences.pullHistoryCount > MAX_PULL_HISTORY) {
    preferences.pullHistoryCount = MAX_PULL_HISTORY;
  }
  
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    String keyPrefix = "pull" + String(i) + "_";
    preferences.pullHistory[i].driverName = storage.getString((keyPrefix + "driver").c_str(), "");
    preferences.pullHistory[i].driverNumber = storage.getInt((keyPrefix + "driverNum").c_str(), 0);
    preferences.pullHistory[i].className = storage.getString((keyPrefix + "class").c_str(), "");
    preferences.pullHistory[i].classWeight = storage.getInt((keyPrefix + "weight").c_str(), 0);
    preferences.pullHistory[i].maxSpeedMPH = storage.getFloat((keyPrefix + "speed").c_str(), 0.0f);
    preferences.pullHistory[i].maxDistanceFeet = storage.getFloat((keyPrefix + "distance").c_str(), 0.0f);
    preferences.pullHistory[i].maxRPM = storage.getFloat((keyPrefix + "rpm").c_str(), 0.0f);
    preferences.pullHistory[i].timestamp = storage.getULong((keyPrefix + "time").c_str(), 0);
  }

  storage.end();
  Serial.printf("[Prefs] LimitSwitchEnabled: LS1 = %d, LS2 = %d\n", preferences.limitSwitchEnabled[0],
                preferences.limitSwitchEnabled[1]);

  // Print pull history
  Serial.printf("[Prefs] Pull History (%d pulls):\n", preferences.pullHistoryCount);
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    const PullResult& pull = preferences.pullHistory[i];
    Serial.printf("  %d: Driver %s (#%d), Speed: %.2f MPH, Distance: %.2f ft, RPM: %.1f, Time: %lu\n", 
                  i, pull.driverName.c_str(), pull.driverNumber, pull.maxSpeedMPH, 
                  pull.maxDistanceFeet, pull.maxRPM, pull.timestamp);
  }

  Serial.println("Preferences loaded successfully.");
}

void StateManager::savePreferences() {
  storage.begin("m4prefs", false);

  storage.putUChar("unitSystem", static_cast<uint8_t>(preferences.unitSystem));
  storage.putString("className", preferences.pullingClassName);
  storage.putInt("classWeight", preferences.pullingClassWeight);
  storage.putString("driverName", preferences.driverName);
  storage.putInt("driverNumber", preferences.driverNumber);

  storage.putBool("ls1_enabled", preferences.limitSwitchEnabled[0]);
  storage.putBool("ls2_enabled", preferences.limitSwitchEnabled[1]);

  for (int i = 0; i < 4; ++i) {
    storage.putBool(("relayEn" + String(i)).c_str(), preferences.relayEnabled[i]);
  }

  storage.putFloat("distAlarm1", preferences.distanceAlarm1);
  storage.putFloat("distAlarm2", preferences.distanceAlarm2);
  storage.putFloat("tachAlarm1", preferences.tachAlarm1);
  storage.putFloat("tachAlarm2", preferences.tachAlarm2);
  storage.putFloat("mphAlarm1", preferences.mphAlarm1);
  storage.putFloat("mphAlarm2", preferences.mphAlarm2);

  storage.putFloat("trackLength", preferences.trackLengthFeet);
  storage.putBool("benchmark", preferences.benchmarkMode);
  storage.putUChar("brightness", preferences.screenBrightness);
  storage.putBool("tachEnabled", preferences.tachEnabled);
  storage.putBool("limitsEnabled", preferences.limitSwitchesEnabled);
  storage.putBool("relaysEnabled", preferences.relaysEnabled);
  storage.putInt("speedCal", preferences.speedCalibrationPulses);

  // Save pull history
  storage.putInt("pullCount", preferences.pullHistoryCount);
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    String keyPrefix = "pull" + String(i) + "_";
    storage.putString((keyPrefix + "driver").c_str(), preferences.pullHistory[i].driverName);
    storage.putInt((keyPrefix + "driverNum").c_str(), preferences.pullHistory[i].driverNumber);
    storage.putString((keyPrefix + "class").c_str(), preferences.pullHistory[i].className);
    storage.putInt((keyPrefix + "weight").c_str(), preferences.pullHistory[i].classWeight);
    storage.putFloat((keyPrefix + "speed").c_str(), preferences.pullHistory[i].maxSpeedMPH);
    storage.putFloat((keyPrefix + "distance").c_str(), preferences.pullHistory[i].maxDistanceFeet);
    storage.putFloat((keyPrefix + "rpm").c_str(), preferences.pullHistory[i].maxRPM);
    storage.putULong((keyPrefix + "time").c_str(), preferences.pullHistory[i].timestamp);
  }

  storage.end();
}
