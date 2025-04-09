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

  preferences.pullingClassName = storage.getString("className", "Default Class");
  preferences.pullingClassWeight = storage.getInt("classWeight", 0);
  preferences.driverName = storage.getString("driverName", "Driver Name");
  preferences.driverNumber = storage.getInt("driverNumber", 2448);

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

  storage.end();
  Serial.printf("[Prefs] LimitSwitchEnabled: LS1 = %d, LS2 = %d\n", preferences.limitSwitchEnabled[0],
                preferences.limitSwitchEnabled[1]);

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

  storage.end();
}
