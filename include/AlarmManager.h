#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>        // needed for UI dispatcher helpers
#include <Preferences.h> // this module persists its own data

#include "Config.h"
#include "StateManager.h"   // for UnitSystem and PullState

// ----------------------------
// Enums
// ----------------------------
enum class AlarmChannel : uint8_t { DISTANCE = 0, SPEED = 1, RPM = 2 };
enum class AlarmSlot    : uint8_t { A1 = 0, A2 = 1 };
enum class AlarmField   : uint8_t { ENABLE = 0, TRIPPOINT = 1, STYLE = 2, COLOR = 3 };

enum class AlarmStyle : uint8_t {
  SILENT = 0,
  TRIP_ONCE = 1,
  HOLD_AUTORESET = 2,
  HOLD_PERSISTENT = 3,
  AUTO_END_RUN_DQ = 4,
};

enum class AlarmColor : uint8_t {
  RED = 0,
  YELLOW = 1,
  GREEN = 2,
};

// ----------------------------
// Constants
// ----------------------------
static const uint8_t  kPresetCount = 4;
static const uint8_t  kChannels = 3;    // DISTANCE, SPEED, RPM
static const uint8_t  kSlots    = 2;    // A1, A2

// Base unit rules for storage:
// - Distance stored in feet
// - Speed stored in mph
// - RPM stored raw

// ----------------------------
// Data model
// ----------------------------
struct AlarmConfig {
  bool        enabled = false;
  float       tripPoint = 0.0f;    // base units (ft, mph, rpm)
  AlarmStyle  style = AlarmStyle::HOLD_AUTORESET;
  AlarmColor  color = AlarmColor::YELLOW;

  // Runtime
  bool        tripped = false;
  bool        firedOnce = false;
  uint32_t    trippedAtMs = 0;
  uint32_t    silencedUntilMs = 0;
  bool        hornSilenced = false;
};

struct AlarmPreset {
  // [channel][slot]
  AlarmConfig alarms[kChannels][kSlots];
  char        name[12] = "Preset";
};

class AlarmManager {
 public:
  // Lifecycle
  static void init();
  static void loadPrefs();
  static void savePrefs();

  // Preset selection
  static uint8_t getActivePreset();
  static void    setActivePreset(uint8_t idx);

  // Accessors by preset
  static AlarmConfig getConfig(uint8_t preset, AlarmChannel ch, AlarmSlot sl);
  static void setEnabled(uint8_t preset, AlarmChannel ch, AlarmSlot sl, bool en);
  static void setTripPoint(uint8_t preset, AlarmChannel ch, AlarmSlot sl, float baseValue);
  static void setStyle(uint8_t preset, AlarmChannel ch, AlarmSlot sl, AlarmStyle st);
  static void setColor(uint8_t preset, AlarmChannel ch, AlarmSlot sl, AlarmColor col);

  // Convenience for active preset
  static AlarmConfig getConfigActive(AlarmChannel ch, AlarmSlot sl);
  static uint8_t getActiveTripMask();
  static void applyMirrorConfig(uint8_t activePreset, const AlarmConfig configs[kChannels][kSlots]);
  static void applyMirrorTripMask(uint8_t activePreset, uint8_t tripMask);

  // Evaluation
  static void evaluateTick(); // call during STAGED and PULLING
  static void tick(); // advances non-blocking horn patterns
  static void resetForStateEntry(); // call on STAGED entry, etc.

  // Horn and silence
  static void silenceForMs(uint32_t ms);
  static bool silenceActiveAudibleAlarms();
  static bool hasActiveAudibleAlarm();

  // UI glue
  static void handleAlarmChange(AlarmChannel ch, AlarmSlot sl, AlarmField field, lv_event_t* e);
  static void handleAlarmPresetChanged(lv_event_t* e);

  // Mapping helpers
  static AlarmStyle mapStyleIndex(int idx);
  static int        mapStyleToIndex(AlarmStyle st);
  static AlarmColor mapColorIndex(int idx);
  static int        mapColorToIndex(AlarmColor c);

  // Unit helpers
  static float uiToBase(AlarmChannel ch, float uiValue);
  static float baseToUi(AlarmChannel ch, float baseValue);

 private:
  static void clampAndValidate(AlarmChannel ch, float& v);

  static Preferences prefs_;
  static AlarmPreset presets_[kPresetCount];
  static uint8_t activePreset_;
  static uint32_t buzzerSilencedUntilMs_;
  static void triggerHorn(const AlarmConfig& A);
  static void enqueueHornPattern(uint8_t pulses, uint16_t onMs, uint16_t offMs);
  static bool isAlarmHornSilenced(const AlarmConfig& A, uint32_t now);

};

#endif // ALARM_MANAGER_H
