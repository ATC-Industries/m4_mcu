#include "AlarmManager.h"
#include "../ui/ui.h"
#include "PullStateManager.h"
#include "peripherals/PeripheralsInit.h"

#define LOG_TAG "AlarmManager"
#include "Logging.h"


// Extern UI objects you already have from Squareline
extern lv_obj_t* uic_Settings1DropdownAlarmPresetDropdown;

// Forward declaration for UI repaint. Implement this in ScreenUpdater.cpp.
extern void refreshAlarmUIFromPreset(uint8_t preset);
extern bool isAlarmUIRefreshing();

// Static storage
Preferences AlarmManager::prefs_;
AlarmPreset AlarmManager::presets_[kPresetCount];
uint8_t     AlarmManager::activePreset_ = 0;
uint32_t    AlarmManager::buzzerSilencedUntilMs_ = 0;

namespace {
// Horn tuning knobs
//
// These constants define the audible "language" of the alarm system. Keep
// them near the top of the file so changing horn feel does not require digging
// through the runtime logic below.
//
// Terms:
// - pulses: how many times the horn turns on for this pattern
// - onMs:   how long each horn-on segment lasts
// - offMs:  silence gap between repeated pulses/patterns
//
// Notes on semantics:
// - TRIP_ONCE is a crossover-only alert: one beep when the value crosses.
// - HOLD_AUTORESET is a continuous horn while the value remains over the trip
//   point. The horn stops as soon as the value drops below, and starts again on
//   the next crossing.
// - HOLD_PERSISTENT re-queues the same pattern while the alarm remains latched,
//   which makes it keep sounding until the run is reset.
// - AUTO_END_RUN_DQ keeps its distinct triple pulse signature.
struct HornPattern {
  uint8_t pulses = 0;
  uint16_t onMs = 0;
  uint16_t offMs = 0;
};

constexpr uint8_t  kHornPatternTripOncePulses       = 1;
constexpr uint16_t kHornPatternTripOnceOnMs         = 180;
constexpr uint16_t kHornPatternTripOnceOffMs        = 0;

constexpr uint8_t  kHornPatternHoldAutoResetPulses  = 0;
constexpr uint16_t kHornPatternHoldAutoResetOnMs    = 0;
constexpr uint16_t kHornPatternHoldAutoResetOffMs   = 0;

constexpr uint8_t  kHornPatternHoldPersistentPulses = 0;
constexpr uint16_t kHornPatternHoldPersistentOnMs   = 0;
constexpr uint16_t kHornPatternHoldPersistentOffMs  = 0;

constexpr uint8_t  kHornPatternDqPulses             = 3;
constexpr uint16_t kHornPatternDqOnMs               = 120;
constexpr uint16_t kHornPatternDqOffMs              = 80;

constexpr uint8_t kHornPatternQueueSize = 8;
HornPattern s_hornQueue[kHornPatternQueueSize];
uint8_t s_hornQueueHead = 0;
uint8_t s_hornQueueTail = 0;
uint8_t s_hornQueueCount = 0;

HornPattern s_activeHornPattern;
uint8_t s_remainingPulses = 0;
bool s_hornIsOn = false;
uint32_t s_hornPhaseUntilMs = 0;
bool s_continuousHornRequested = false;
bool s_continuousHornActive = false;

void resetHornScheduler() {
  g_horn.off();
  s_hornQueueHead = 0;
  s_hornQueueTail = 0;
  s_hornQueueCount = 0;
  s_activeHornPattern = {};
  s_remainingPulses = 0;
  s_hornIsOn = false;
  s_hornPhaseUntilMs = 0;
  s_continuousHornRequested = false;
  s_continuousHornActive = false;
}

bool dequeueHornPattern(HornPattern& out) {
  if (s_hornQueueCount == 0) {
    return false;
  }

  out = s_hornQueue[s_hornQueueHead];
  s_hornQueueHead = static_cast<uint8_t>((s_hornQueueHead + 1) % kHornPatternQueueSize);
  --s_hornQueueCount;
  return true;
}

bool hornPatternMatches(const HornPattern& pattern, uint8_t pulses, uint16_t onMs, uint16_t offMs) {
  return pattern.pulses == pulses && pattern.onMs == onMs && pattern.offMs == offMs;
}

void clearAlarmHornSilence(AlarmConfig& alarm) {
  alarm.silencedUntilMs = 0;
  alarm.hornSilenced = false;
}

HornPattern hornPatternForStyle(AlarmStyle style) {
  switch (style) {
    case AlarmStyle::TRIP_ONCE:
      return HornPattern{kHornPatternTripOncePulses,
                         kHornPatternTripOnceOnMs,
                         kHornPatternTripOnceOffMs};
    case AlarmStyle::HOLD_AUTORESET:
      return HornPattern{kHornPatternHoldAutoResetPulses,
                         kHornPatternHoldAutoResetOnMs,
                         kHornPatternHoldAutoResetOffMs};
    case AlarmStyle::HOLD_PERSISTENT:
      return HornPattern{kHornPatternHoldPersistentPulses,
                         kHornPatternHoldPersistentOnMs,
                         kHornPatternHoldPersistentOffMs};
    case AlarmStyle::AUTO_END_RUN_DQ:
      return HornPattern{kHornPatternDqPulses,
                         kHornPatternDqOnMs,
                         kHornPatternDqOffMs};
    case AlarmStyle::SILENT:
    default:
      return HornPattern{};
  }
}
}  // namespace

// ----------------------------
// Lifecycle
// ----------------------------
void AlarmManager::init() {
  loadPrefs();
}

void AlarmManager::loadPrefs() {
  LOGI("Loading preferences...");
  
  if (!prefs_.begin("m4alarms", false)) {
    LOGW("prefs begin failed, using defaults");
    // Seed a simple default
    for (int p = 0; p < kPresetCount; ++p) {
      snprintf(presets_[p].name, sizeof(presets_[p].name), "Preset %d", p + 1);
      LOGD("Default preset %d: '%s'", p, presets_[p].name);
      
      for (int c = 0; c < kChannels; ++c) {
        for (int s = 0; s < kSlots; ++s) {
          AlarmConfig& A = presets_[p].alarms[c][s];
          A.enabled = (p == 0); // enable only for first preset by default
          A.tripPoint = 0;
          A.style = AlarmStyle::HOLD_AUTORESET;
          A.color = AlarmColor::YELLOW;
        }
      }
    }
    activePreset_ = 0;
    LOGI("Defaults initialized");
    return;
  }

  activePreset_ = prefs_.getUChar("active", 0);
  if (activePreset_ >= kPresetCount) activePreset_ = 0;
  LOGI("Active preset: %d", activePreset_);

  for (int p = 0; p < kPresetCount; ++p) {
    char key[24];
    
    // name
    snprintf(key, sizeof(key), "p%d_name", p);
    String nm = prefs_.getString(key, String("Preset ") + String(p + 1));
    strncpy(presets_[p].name, nm.c_str(), sizeof(presets_[p].name));
    presets_[p].name[sizeof(presets_[p].name) - 1] = '\0';
    LOGD("Preset %d name: '%s'", p, presets_[p].name);

    for (int c = 0; c < kChannels; ++c) {
      for (int s = 0; s < kSlots; ++s) {
        AlarmConfig& A = presets_[p].alarms[c][s];

        snprintf(key, sizeof(key), "p%d_%d_%d_en", p, c, s);
        A.enabled = prefs_.getBool(key, p == 0);

        snprintf(key, sizeof(key), "p%d_%d_%d_tp", p, c, s);
        A.tripPoint = prefs_.getFloat(key, 0.0f);

        snprintf(key, sizeof(key), "p%d_%d_%d_st", p, c, s);
        A.style = static_cast<AlarmStyle>(prefs_.getUChar(key, (uint8_t)AlarmStyle::HOLD_AUTORESET));

        snprintf(key, sizeof(key), "p%d_%d_%d_co", p, c, s);
        A.color = static_cast<AlarmColor>(prefs_.getUChar(key, (uint8_t)AlarmColor::YELLOW));

        // Log if this alarm is enabled or has non-zero trip point
        if (A.enabled || A.tripPoint != 0.0f) {
          LOGD("P%d Ch%d Slot%d: en=%d tp=%.2f st=%d co=%d",
               p, c, s, A.enabled, A.tripPoint, (int)A.style, (int)A.color);
        }

        // runtime cleared
        A.tripped = false;
        A.firedOnce = false;
        A.trippedAtMs = 0;
        clearAlarmHornSilence(A);
      }
    }
  }

  refreshAlarmUIFromPreset(getActivePreset());

  prefs_.end();
  LOGI("prefs loaded successfully");
}

void AlarmManager::savePrefs() {
  if (!prefs_.begin("m4alarms", false)) {
    LOGE("prefs save begin failed");
    return;
  }

  prefs_.putUChar("active", activePreset_);

  for (int p = 0; p < kPresetCount; ++p) {
    char key[24];
    // name
    snprintf(key, sizeof(key), "p%d_name", p);
    prefs_.putString(key, presets_[p].name);

    for (int c = 0; c < kChannels; ++c) {
      for (int s = 0; s < kSlots; ++s) {
        AlarmConfig& A = presets_[p].alarms[c][s];

        snprintf(key, sizeof(key), "p%d_%d_%d_en", p, c, s);
        prefs_.putBool(key, A.enabled);

        snprintf(key, sizeof(key), "p%d_%d_%d_tp", p, c, s);
        prefs_.putFloat(key, A.tripPoint);

        snprintf(key, sizeof(key), "p%d_%d_%d_st", p, c, s);
        prefs_.putUChar(key, (uint8_t)A.style);

        snprintf(key, sizeof(key), "p%d_%d_%d_co", p, c, s);
        prefs_.putUChar(key, (uint8_t)A.color);
      }
    }
  }

  prefs_.end();
  refreshAlarmUIFromPreset(getActivePreset());
  LOGI("prefs saved");
}

// ----------------------------
// Presets
// ----------------------------
uint8_t AlarmManager::getActivePreset() { return activePreset_; }

void AlarmManager::setActivePreset(uint8_t idx) {
  if (idx >= kPresetCount) idx = 0;
  if (activePreset_ == idx) return;
  activePreset_ = idx;
  savePrefs(); // keep simple for now
}

AlarmConfig AlarmManager::getConfig(uint8_t preset, AlarmChannel ch, AlarmSlot sl) {
  if (preset >= kPresetCount) preset = 0;
  return presets_[preset].alarms[(int)ch][(int)sl];
}

AlarmConfig AlarmManager::getConfigActive(AlarmChannel ch, AlarmSlot sl) {
  return getConfig(activePreset_, ch, sl);
}

uint8_t AlarmManager::getActiveTripMask() {
  uint8_t mask = 0;
  for (int c = 0; c < kChannels; ++c) {
    for (int s = 0; s < kSlots; ++s) {
      if (presets_[activePreset_].alarms[c][s].tripped) {
        mask |= static_cast<uint8_t>(1U << (c * kSlots + s));
      }
    }
  }
  return mask;
}

void AlarmManager::applyMirrorConfig(uint8_t activePreset,
                                     const AlarmConfig configs[kChannels][kSlots]) {
  if (activePreset >= kPresetCount) activePreset = 0;
  bool changed = activePreset_ != activePreset;
  activePreset_ = activePreset;

  for (int c = 0; c < kChannels; ++c) {
    for (int s = 0; s < kSlots; ++s) {
      AlarmConfig& dst = presets_[activePreset_].alarms[c][s];
      const AlarmConfig& src = configs[c][s];

      if (dst.enabled != src.enabled ||
          fabs(dst.tripPoint - src.tripPoint) > 0.001f ||
          dst.style != src.style ||
          dst.color != src.color) {
        changed = true;
      }

      // Mirror only the host's persistent config here. Runtime trip state is
      // mirrored separately by the fast telemetry packets and should not be
      // cleared by the slower config refresh.
      dst.enabled = src.enabled;
      dst.tripPoint = src.tripPoint;
      dst.style = src.style;
      dst.color = src.color;
    }
  }

  if (changed) {
    refreshAlarmUIFromPreset(activePreset_);
  }
}

void AlarmManager::applyMirrorTripMask(uint8_t activePreset, uint8_t tripMask) {
  if (activePreset >= kPresetCount) activePreset = 0;
  activePreset_ = activePreset;

  for (int c = 0; c < kChannels; ++c) {
    for (int s = 0; s < kSlots; ++s) {
      AlarmConfig& alarm = presets_[activePreset_].alarms[c][s];
      const bool tripped = (tripMask & (1U << (c * kSlots + s))) != 0;
      alarm.tripped = tripped;
      if (!tripped) {
        alarm.firedOnce = false;
        alarm.trippedAtMs = 0;
        clearAlarmHornSilence(alarm);
      }
    }
  }
}

void AlarmManager::setEnabled(uint8_t preset, AlarmChannel ch, AlarmSlot sl, bool en) {
  if (preset >= kPresetCount) preset = 0;
  presets_[preset].alarms[(int)ch][(int)sl].enabled = en;
  savePrefs();
}

void AlarmManager::setTripPoint(uint8_t preset, AlarmChannel ch, AlarmSlot sl, float baseValue) {
  LOGI("setTripPoint called");
  if (preset >= kPresetCount) preset = 0;
  float v = baseValue;
  clampAndValidate(ch, v);
  presets_[preset].alarms[(int)ch][(int)sl].tripPoint = v;
  savePrefs();
  LOGI("Trip point set to %.2f for preset %d", v, preset);
}

void AlarmManager::setStyle(uint8_t preset, AlarmChannel ch, AlarmSlot sl, AlarmStyle st) {
  if (preset >= kPresetCount) preset = 0;
  presets_[preset].alarms[(int)ch][(int)sl].style = st;
  // Reset runtime flags on style change
  AlarmConfig& A = presets_[preset].alarms[(int)ch][(int)sl];
  A.tripped = false; A.firedOnce = false; A.trippedAtMs = 0;
  clearAlarmHornSilence(A);
  savePrefs();
}

void AlarmManager::setColor(uint8_t preset, AlarmChannel ch, AlarmSlot sl, AlarmColor col) {
  if (preset >= kPresetCount) preset = 0;
  presets_[preset].alarms[(int)ch][(int)sl].color = col;
  savePrefs();
}

// ----------------------------
// Evaluation
// ----------------------------
void AlarmManager::evaluateTick() {
  if (StateManager::getJudgeMode()) {
    s_continuousHornRequested = false;
    return;
  }

  PullState pullState = StateManager::getPullState();
  
  // Only evaluate during STAGED and PULLING
  if (pullState != PullState::STAGED && pullState != PullState::PULLING) {
    s_continuousHornRequested = false;
    return;
  }
  
  uint32_t now = millis();
  const bool globalHornSilenced = now < buzzerSilencedUntilMs_;
  
  // Get current values (in base units)
  float distFt = StateManager::state().distanceInFeet;
  float speedMph = StateManager::state().speedInMPH;
  float rpm = StateManager::state().rpm;
  
  // Log current values occasionally (every 50 calls to avoid spam)
  static int logCounter = 0;
  if (++logCounter >= 500) {
    LOGD("Current values: RPM=%.0f, Speed=%.1f, Dist=%.2f", rpm, speedMph, distFt);
    logCounter = 0;
  }
  
  AlarmPreset& P = presets_[activePreset_];
  bool holdAutoResetActive = false;
  
  // Evaluate all alarms
  for (int c = 0; c < kChannels; ++c) {
    AlarmChannel ch = static_cast<AlarmChannel>(c);
    
    // Get current value for this channel
    float currentValue = 0.0f;
    switch (ch) {
      case AlarmChannel::DISTANCE: currentValue = distFt; break;
      case AlarmChannel::SPEED:    currentValue = speedMph; break;
      case AlarmChannel::RPM:      currentValue = rpm; break;
    }
    
    for (int s = 0; s < kSlots; ++s) {
      AlarmConfig& A = P.alarms[c][s];
      
      if (!A.enabled) {
        A.tripped = false;
        A.firedOnce = false;
        A.trippedAtMs = 0;
        clearAlarmHornSilence(A);
        continue;
      }
      
      // Check if value exceeds trip point
      bool shouldTrip = (currentValue >= A.tripPoint);
      
      // Apply style logic
      switch (A.style) {
        case AlarmStyle::SILENT:
          // No horn, but set tripped state for UI
          A.tripped = shouldTrip;
          if (!shouldTrip) {
            A.firedOnce = false;
            A.trippedAtMs = 0;
            clearAlarmHornSilence(A);
          }
          break;
          
        case AlarmStyle::TRIP_ONCE:
          if (shouldTrip && !A.firedOnce) {
            A.tripped = true;
            A.firedOnce = true;
            A.trippedAtMs = now;
            clearAlarmHornSilence(A);
            LOGI("TRIP_ONCE fired: ch=%d slot=%d value=%.2f tripPoint=%.2f", 
                 (int)ch, s, currentValue, A.tripPoint);
            triggerHorn(A);
          } else if (!shouldTrip) {
            A.tripped = false;
            A.firedOnce = false;
            A.trippedAtMs = 0;
            clearAlarmHornSilence(A);
          }
          break;
          
        case AlarmStyle::HOLD_AUTORESET:
          if (shouldTrip) {
            if (!A.tripped) {
              A.trippedAtMs = now;
              clearAlarmHornSilence(A);
              LOGI("HOLD_AUTORESET fired: ch=%d slot=%d value=%.2f tripPoint=%.2f", 
                   (int)ch, s, currentValue, A.tripPoint);
            }
            A.tripped = true;
            if (!globalHornSilenced && !isAlarmHornSilenced(A, now)) {
              holdAutoResetActive = true;
            }
          } else {
            // Auto-reset when value drops below trip point
            if (A.tripped) {
              LOGD("HOLD_AUTORESET reset: ch=%d slot=%d", (int)ch, s);
            }
            A.tripped = false;
            A.firedOnce = false;
            A.trippedAtMs = 0;
            clearAlarmHornSilence(A);
          }
          break;
          
        case AlarmStyle::HOLD_PERSISTENT:
          if (shouldTrip) {
            if (!A.tripped) {
              A.tripped = true;
              A.trippedAtMs = now;
              clearAlarmHornSilence(A);
              LOGI("HOLD_PERSISTENT fired: ch=%d slot=%d value=%.2f tripPoint=%.2f", 
                   (int)ch, s, currentValue, A.tripPoint);
            }
            triggerHorn(A);
          }
          // Never auto-reset - stays tripped until manual reset
          break;
          
        case AlarmStyle::AUTO_END_RUN_DQ:
          if (shouldTrip && !A.tripped) {
            A.tripped = true;
            A.trippedAtMs = now;
            clearAlarmHornSilence(A);
            LOGI("AUTO_END_RUN_DQ fired: ch=%d slot=%d value=%.2f tripPoint=%.2f", 
                 (int)ch, s, currentValue, A.tripPoint);
            triggerHorn(A);
            LOGW("AUTO_END_RUN_DQ trip is latched, but automatic PULLEND transition is disabled");
          }
          break;
      }
    }
  }

  s_continuousHornRequested = holdAutoResetActive;
}

void AlarmManager::tick() {
  const uint32_t now = millis();

  if (s_continuousHornRequested) {
    if (!s_continuousHornActive) {
      g_horn.on();
      s_continuousHornActive = true;
    }
    s_hornIsOn = false;
    s_remainingPulses = 0;
    s_hornPhaseUntilMs = 0;
    return;
  }

  if (s_continuousHornActive) {
    g_horn.off();
    s_continuousHornActive = false;
  }

  if (s_remainingPulses == 0) {
    HornPattern nextPattern;
    if (!dequeueHornPattern(nextPattern)) {
      if (s_hornIsOn) {
        g_horn.off();
        s_hornIsOn = false;
      }
      return;
    }

    s_activeHornPattern = nextPattern;
    s_remainingPulses = nextPattern.pulses;
    s_hornIsOn = true;
    g_horn.on();
    s_hornPhaseUntilMs = now + nextPattern.onMs;
    return;
  }

  if (now < s_hornPhaseUntilMs) {
    return;
  }

  if (s_hornIsOn) {
    g_horn.off();
    s_hornIsOn = false;
    --s_remainingPulses;

    if (s_remainingPulses == 0) {
      s_hornPhaseUntilMs = 0;
      return;
    }

    s_hornPhaseUntilMs = now + s_activeHornPattern.offMs;
    return;
  }

  s_hornIsOn = true;
  g_horn.on();
  s_hornPhaseUntilMs = now + s_activeHornPattern.onMs;
}

void AlarmManager::resetForStateEntry() {
  // TODO: call on STAGED entry
  AlarmPreset& P = presets_[activePreset_];
  for (int c = 0; c < kChannels; ++c) {
    for (int s = 0; s < kSlots; ++s) {
      AlarmConfig& A = P.alarms[c][s];
      A.tripped = false;
      A.firedOnce = false;
      A.trippedAtMs = 0;
      clearAlarmHornSilence(A);
    }
  }
  buzzerSilencedUntilMs_ = 0;
  resetHornScheduler();
}

// ----------------------------
// Horn and silence
// ----------------------------
void AlarmManager::silenceForMs(uint32_t ms) {
  buzzerSilencedUntilMs_ = millis() + ms;
}

bool AlarmManager::silenceActiveAudibleAlarms() {
  AlarmPreset& preset = presets_[activePreset_];
  bool silencedAny = false;

  for (int c = 0; c < kChannels; ++c) {
    for (int s = 0; s < kSlots; ++s) {
      AlarmConfig& alarm = preset.alarms[c][s];
      if (!alarm.enabled || !alarm.tripped || alarm.style == AlarmStyle::SILENT) {
        continue;
      }

      alarm.hornSilenced = true;
      alarm.silencedUntilMs = UINT32_MAX;
      silencedAny = true;
    }
  }

  if (!silencedAny) {
    return false;
  }

  s_continuousHornRequested = false;
  resetHornScheduler();
  LOGI("Silenced active alarm horns until their next reset/trip cycle");
  return true;
}

bool AlarmManager::hasActiveAudibleAlarm() {
  return s_continuousHornRequested ||
         s_continuousHornActive ||
         s_hornIsOn ||
         s_remainingPulses > 0 ||
         s_hornQueueCount > 0;
}

// ----------------------------
// UI glue
// ----------------------------
static int getActivePresetFromButtons() {
  // Fallback to whatever is already stored if UI not built yet
  if (!uic_Settings1ButtonPresetButton1) 
      return AlarmManager::getActivePreset();

  if (lv_obj_has_state(uic_Settings1ButtonPresetButton1, LV_STATE_CHECKED)) return 0;
  if (lv_obj_has_state(uic_Settings1ButtonPresetButton2, LV_STATE_CHECKED)) return 1;
  if (lv_obj_has_state(uic_Settings1ButtonPresetButton3, LV_STATE_CHECKED)) return 2;
  if (lv_obj_has_state(uic_Settings1ButtonPresetButton4, LV_STATE_CHECKED)) return 3;

  LOGW("no preset button checked");

  // fallback: none checked, return last known preset
  return AlarmManager::getActivePreset();
}

void AlarmManager::handleAlarmChange(AlarmChannel ch, AlarmSlot sl, AlarmField field, lv_event_t* e) {  
  const uint8_t preset = (uint8_t)getActivePresetFromButtons();
  
  const lv_event_code_t code = lv_event_get_code(e);
  
  if (isAlarmUIRefreshing()) {
    return;
  }

  switch (field) {
    case AlarmField::ENABLE:
    case AlarmField::STYLE:
    case AlarmField::COLOR:
      if (code != LV_EVENT_VALUE_CHANGED) return;
      break;
    case AlarmField::TRIPPOINT:
      if (code != LV_EVENT_DEFOCUSED && code != LV_EVENT_READY) {;
        return;
      }
      break;
  }

  lv_obj_t* target = lv_event_get_target(e);
  if (!target) {
    return;
  }

  switch (field) {
    case AlarmField::ENABLE: {
      bool en = lv_obj_has_state(target, LV_STATE_CHECKED);
      setEnabled(preset, ch, sl, en);
    } break;

    case AlarmField::TRIPPOINT: {
      LOGI("TRIPPOINT case entered");
      LOGI("Preset: %d, Channel: %d, Slot: %d", preset, (int)ch, (int)sl);
      
      const char* txt = lv_textarea_get_text(target);
      if (!txt) {
        return;
      }
      
      // parse
      char* endp = nullptr;
      float v = strtof(txt, &endp);
      if (endp == txt || isnan(v)) {
        LOGW("Invalid parse, refreshing UI");
        refreshAlarmUIFromPreset(preset);
        return;
      }
      
      LOGI("Parsed value: %.2f", v);
      
      // convert UI units to base
      float base = uiToBase(ch, v);
      
      setTripPoint(preset, ch, sl, base);
      refreshAlarmUIFromPreset(preset);
    } break;

    case AlarmField::STYLE: {
      int idx = lv_dropdown_get_selected(target);
      setStyle(preset, ch, sl, mapStyleIndex(idx));
    } break;

    case AlarmField::COLOR: {
      int idx = lv_dropdown_get_selected(target);
      setColor(preset, ch, sl, mapColorIndex(idx));
      refreshAlarmUIFromPreset(preset);
    } break;
  }
}

// Call this from your preset dropdown LV_EVENT_VALUE_CHANGED
void AlarmManager::handleAlarmPresetChanged(lv_event_t* /*e*/) {
  const uint8_t preset = (uint8_t)getActivePresetFromButtons();
  setActivePreset(preset);
  refreshAlarmUIFromPreset(preset);
}

// ----------------------------
// Mapping helpers
// ----------------------------
AlarmStyle AlarmManager::mapStyleIndex(int idx) {
  switch (idx) {
    case 0: return AlarmStyle::SILENT;
    case 1: return AlarmStyle::TRIP_ONCE;
    case 2: return AlarmStyle::HOLD_AUTORESET;
    case 3: return AlarmStyle::HOLD_PERSISTENT;
    case 4: return AlarmStyle::AUTO_END_RUN_DQ;
    default: return AlarmStyle::HOLD_AUTORESET;
  }
}

int AlarmManager::mapStyleToIndex(AlarmStyle st) {
  switch (st) {
    case AlarmStyle::SILENT:           return 0;
    case AlarmStyle::TRIP_ONCE:        return 1;
    case AlarmStyle::HOLD_AUTORESET:   return 2;
    case AlarmStyle::HOLD_PERSISTENT:  return 3;
    case AlarmStyle::AUTO_END_RUN_DQ:  return 4;
  }
  return 2;
}

AlarmColor AlarmManager::mapColorIndex(int idx) {
  switch (idx) {
    case 0: return AlarmColor::RED;
    case 1: return AlarmColor::YELLOW;
    case 2: return AlarmColor::GREEN;
    default: return AlarmColor::YELLOW;
  }
}

int AlarmManager::mapColorToIndex(AlarmColor c) {
  switch (c) {
    case AlarmColor::RED:    return 0;
    case AlarmColor::YELLOW: return 1;
    case AlarmColor::GREEN:  return 2;
  }
  return 1;
}

// ----------------------------
// Unit helpers
// ----------------------------
float AlarmManager::uiToBase(AlarmChannel ch, float uiValue) {
  UnitSystem us = StateManager::getUnitSystem();
  switch (ch) {
    case AlarmChannel::DISTANCE:
      return (us == UnitSystem::METRIC) ? (uiValue / 0.3048f) : uiValue;     // m -> ft
    case AlarmChannel::SPEED:
      return (us == UnitSystem::METRIC) ? (uiValue / 1.60934f) : uiValue;     // km/h -> mph
    case AlarmChannel::RPM:
      return uiValue;
  }
  return uiValue;
}

float AlarmManager::baseToUi(AlarmChannel ch, float baseValue) {
  UnitSystem us = StateManager::getUnitSystem();
  switch (ch) {
    case AlarmChannel::DISTANCE:
      return (us == UnitSystem::METRIC) ? (baseValue * 0.3048f) : baseValue;  // ft -> m
    case AlarmChannel::SPEED:
      return (us == UnitSystem::METRIC) ? (baseValue * 1.60934f) : baseValue; // mph -> km/h
    case AlarmChannel::RPM:
      return baseValue;
  }
  return baseValue;
}

// ----------------------------
// Validation and clamping
// ----------------------------
void AlarmManager::clampAndValidate(AlarmChannel ch, float& v) {
  if (v < 0) v = 0;
  switch (ch) {
    case AlarmChannel::DISTANCE: {
      float trackFt = StateManager::prefs().trackLengthFeet; // base units here
      if (v > trackFt * 1.25f) v = trackFt * 1.25f;
    } break;
    case AlarmChannel::SPEED:
      if (v > 300.0f) v = 300.0f;
      break;
    case AlarmChannel::RPM:
      if (v > 10000.0f) v = 10000.0f;
      break;
  }
}

void AlarmManager::triggerHorn(const AlarmConfig& A) {
  uint32_t now = millis();
  
  // Check global silence
  if (now < buzzerSilencedUntilMs_) {
    LOGD("Horn silenced globally until %lu ms", buzzerSilencedUntilMs_);
    return;
  }
  
  // Check per-alarm silence
  if (isAlarmHornSilenced(A, now)) {
    LOGD("Horn silenced for this alarm until %lu ms", A.silencedUntilMs);
    return;
  }
  
  // Log which alarm is firing
  const char* styleName = "UNKNOWN";
  switch (A.style) {
    case AlarmStyle::SILENT: styleName = "SILENT"; break;
    case AlarmStyle::TRIP_ONCE: styleName = "TRIP_ONCE"; break;
    case AlarmStyle::HOLD_AUTORESET: styleName = "HOLD_AUTORESET"; break;
    case AlarmStyle::HOLD_PERSISTENT: styleName = "HOLD_PERSISTENT"; break;
    case AlarmStyle::AUTO_END_RUN_DQ: styleName = "AUTO_END_RUN_DQ"; break;
  }
  
  const char* colorName = "UNKNOWN";
  switch (A.color) {
    case AlarmColor::RED: colorName = "RED"; break;
    case AlarmColor::YELLOW: colorName = "YELLOW"; break;
    case AlarmColor::GREEN: colorName = "GREEN"; break;
  }
  
  LOGI("HORN TRIGGERED: TripPoint=%.2f Style=%s Color=%s Enabled=%d", 
       A.tripPoint, styleName, colorName, A.enabled);

  const HornPattern pattern = hornPatternForStyle(A.style);
  if (pattern.pulses == 0) {
    return;
  }

  enqueueHornPattern(pattern.pulses, pattern.onMs, pattern.offMs);
}

bool AlarmManager::isAlarmHornSilenced(const AlarmConfig& A, uint32_t now) {
  return A.hornSilenced || now < A.silencedUntilMs;
}

void AlarmManager::enqueueHornPattern(uint8_t pulses, uint16_t onMs, uint16_t offMs) {
  if (pulses == 0) {
    return;
  }

  if (s_hornQueueCount >= kHornPatternQueueSize) {
    LOGW("Horn pattern queue full, dropping pattern pulses=%u on=%u off=%u",
         pulses, onMs, offMs);
    return;
  }

  if (s_remainingPulses > 0 && hornPatternMatches(s_activeHornPattern, pulses, onMs, offMs)) {
    return;
  }

  if (s_hornQueueCount > 0) {
    const uint8_t lastIndex = static_cast<uint8_t>((s_hornQueueTail + kHornPatternQueueSize - 1) % kHornPatternQueueSize);
    if (hornPatternMatches(s_hornQueue[lastIndex], pulses, onMs, offMs)) {
      return;
    }
  }

  s_hornQueue[s_hornQueueTail] = HornPattern{pulses, onMs, offMs};
  s_hornQueueTail = static_cast<uint8_t>((s_hornQueueTail + 1) % kHornPatternQueueSize);
  ++s_hornQueueCount;
}
