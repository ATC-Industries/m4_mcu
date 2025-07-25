#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <Arduino.h>

#include "Config.h"

//
// ENUMS
//

enum class UnitSystem { IMPERIAL, METRIC };
enum class LimitSwitchTriggerMode { MAKE, BREAK, LSM_DISABLED };
enum class PullState { READY, STAGED, PULLING, PULLEND, EMERGENCYSTOP };
enum class RelayState { ENGAGED, DISENGAGED };

//
// STRUCTS
//

struct PullResult {
  String driverName;
  int driverNumber;
  String className;
  int classWeight;
  float maxSpeedMPH;
  float maxDistanceFeet;
  float maxRPM;
  unsigned long timestamp;  // Unix timestamp when pull was saved
};

struct SystemPreferences {
  UnitSystem unitSystem = UnitSystem::IMPERIAL;

  String pullingClassName = "M4 Sled Monitor - " + String(VERSION);
  int pullingClassWeight = 0;
  String driverName = "Driver";
  int driverNumber = 1;

  bool limitSwitchEnabled[2] = {true, true};

  bool relayEnabled[4] = {true, true, true, true};

  float distanceAlarm1 = 0.0f;
  float distanceAlarm2 = 0.0f;
  float tachAlarm1 = 0.0f;
  float tachAlarm2 = 0.0f;
  float mphAlarm1 = 0.0f;
  float mphAlarm2 = 0.0f;

  float trackLengthFeet = 300.0f;

  bool benchmarkMode = false;
  uint8_t screenBrightness = 100;
  bool tachEnabled = true;
  bool limitSwitchesEnabled = true;
  bool relaysEnabled = true;

  int speedCalibrationPulses = 1000;

  // Pull history
  PullResult pullHistory[MAX_PULL_HISTORY];
  int pullHistoryCount = 0;  // Number of saved pulls (0 to MAX_PULL_HISTORY)
};

struct SystemState {
  float distanceInFeet = 0.0f;
  float maxDistanceInFeet = 0.0f;

  float speedInMPH = 0.0f;
  float maxSpeedInMPH = 0.0f;

  float rpm = 0.0f;
  float maxRpm = 0.0f;

  bool limitSwitchTriggered[2] = {false, false};

  RelayState relayStates[4] = {RelayState::DISENGAGED, RelayState::DISENGAGED, RelayState::DISENGAGED,
                               RelayState::DISENGAGED};

  PullState currentPullState = PullState::READY;
};

//
// CLASS
//

class StateManager {
public:
  static SystemState& state();
  static SystemPreferences& prefs();

  // Unit-aware getters
  static UnitSystem getUnitSystem();
  static float getDistance();     // Converts to meters if metric
  static float getSpeed();        // Converts to km/h if metric
  static float getRPM();          // Returns RPM
  static float getTrackLength();  // Converts to meters if metric
  static float getMaxRPM();       // Returns max RPM
  static float getMaxSpeed();     // Converts to km/h if metric
  static float getMaxDistance();  // Converts to meters if metric

  // State accessors
  static void setUnitSystem(UnitSystem system);

  static void setDistance(float ft);  // Sets distance in feet
  static void setSpeed(float mph);    // Sets speed in mph
  static void setRPM(float rpm);      // Sets RPM

  static void resetMaxRPM();
  static void resetMaxSpeed();
  static void resetMaxDistance();
  static void resetAllMaxValues();

  static bool getLimitSwitchTriggered(int index);
  static void setLimitSwitchTriggered(int index, bool triggered);

  static bool isLimitSwitchEnabled(int index);
  static void setLimitSwitchEnabled(int index, bool enabled);

  static void setPullState(PullState state);
  static PullState getPullState();

  static RelayState getRelayState(int index);
  static void setRelayState(int index, RelayState state);

  static int getSpeedCalibrationNumber();
  static void setSpeedCalibrationNumber(int pulses);

  // Pull result management
  static void savePullResult();
  static int getPullHistoryCount();
  static const PullResult* getPullHistory();
  static const PullResult* getPullResult(int index);
  static void clearPullHistory();

  // Persistence
  static void loadPreferences();
  static void savePreferences();

private:
  static SystemState systemState;
  static SystemPreferences preferences;
};

#endif  // STATEMANAGER_H
