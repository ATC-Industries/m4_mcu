#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <Arduino.h>
#include <limits.h>

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
  unsigned long timestamp;  // ticks since last boot.  not great but we dont have an RTC clock on the MCU
};

struct SystemPreferences {
  UnitSystem unitSystem = UnitSystem::IMPERIAL;

  String pullingClassName = "M4 Sled Monitor - " + String(DEVICE_VERSION);
  int pullingClassWeight = 0;
  String driverName = "Driver";
  int driverNumber = 1;
  int M4IDNumber = 0;
  int HostM4IDNumber = 0;
  bool isJudgeMode = false;

  bool limitSwitchEnabled[2] = {true, true};

  bool relayEnabled[4] = {true, true, true, true};

  float distanceAlarm1 = 0.0f;
  float distanceAlarm2 = 0.0f;
  float tachAlarm1 = 0.0f;
  float tachAlarm2 = 0.0f;
  float mphAlarm1 = 0.0f;
  float mphAlarm2 = 0.0f;

  float trackLengthFeet = 300.0f;

  uint8_t screenBrightness = 100;
  bool screenRotation180 = false;  // false = 0°, true = 180°
  bool tachEnabled = true;
  bool limitSwitchesEnabled = true;
  bool relaysEnabled = true;
  bool isAutoConnectTractor = true;

  int speedCalibrationPulses = 1000;

  // Pull history
  PullResult pullHistory[MAX_PULL_HISTORY];
  int pullHistoryCount = 0;  // Number of saved pulls (0 to MAX_PULL_HISTORY)

  // M4 Communication Settings
  uint8_t pairedTractorAddress[6] = {0, 0, 0, 0, 0, 0};
  uint8_t pairedRemoteDisplayAddress[6] = {0, 0, 0, 0, 0, 0};
  unsigned long pairingDelay = 10000;  // Default 10 second pairing delay
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

  bool judgeMode = false;

  int pairedTractorRSSI = INT_MIN;  // Start with the lowest possible value
  uint8_t pairedTractorAddress[6] = {
    0};                  // To store the address of the paired tractor
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
  static bool getScreenRotation();  // Returns current screen rotation state 

  // State accessors
  static void setUnitSystem(UnitSystem system);
  static void setScreenBrightness(uint8_t level);      // 0..255
  static void setTrackLengthFeet(float feet);          // > 0
  static void setTachEnabled(bool on);
  static void setLimitSwitchesEnabled(bool on);
  static void setRelaysEnabled(bool on);


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
  
  static void setScreenRotation(bool rotation180);
  static void setM4ID(int unitId, bool persist = true);
  static void setHostM4ID(int unitId, bool persist = true);
  static void setJudgeMode(bool isJudgeMode, bool persist = true);
  static void setJudgeMirrorUnits(UnitSystem system, float trackLengthFeet);
  static void clearJudgeMirrorUnits();

  // M4 Communication with Tach Tractor methods
  static bool getIsAutoConnectTractor();
  static void setIsAutoConnectTractor(bool autoConnect);
  static const uint8_t* getPairedTractorAddress();
  static void setPairedTractorAddress(const uint8_t* address);

  // M4 Communication with Remote Display methods
  static const uint8_t* getPairedRemoteDisplayAddress();
  static void setPairedRemoteDisplayAddress(const uint8_t* address);
  static unsigned long getPairingDelay();
  static void setPairingDelay(unsigned long delay);

  // Pull result management
  static void savePullResult();
  static int getPullHistoryCount();
  static const PullResult* getPullHistory();
  static const PullResult* getPullResult(int index);
  static void clearPullHistory();

  static int getM4ID();       // Added getter for M4 ID
  static int getHostM4ID();  // Added getter for Host M4 ID
  static bool getJudgeMode();            // runtime flag
  static bool getJudgeModePreference();  // persisted preference

  // Persistence
  static void loadPreferences();
  static void savePreferences();
  static void flushPreferencesNow();
  static void processPendingSave();

private:
  static SystemState systemState;
  static SystemPreferences preferences;
};


#endif  // STATEMANAGER_H
