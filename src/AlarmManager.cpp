#include "AlarmManager.h"
#include "../ui/ui.h"


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

// ----------------------------
// Lifecycle
// ----------------------------
void AlarmManager::init() {
  loadPrefs();
}

void AlarmManager::loadPrefs() {
  Serial.println("[AlarmManager] Loading preferences...");
  
  if (!prefs_.begin("m4alarms", false)) {
    Serial.println("[AlarmManager] prefs begin failed, using defaults");
    // Seed a simple default
    for (int p = 0; p < kPresetCount; ++p) {
      snprintf(presets_[p].name, sizeof(presets_[p].name), "Preset %d", p + 1);
      Serial.printf("  Default preset %d: '%s'\n", p, presets_[p].name);
      
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
    Serial.println("[AlarmManager] Defaults initialized");
    return;
  }

  activePreset_ = prefs_.getUChar("active", 0);
  if (activePreset_ >= kPresetCount) activePreset_ = 0;
  Serial.printf("[AlarmManager] Active preset: %d\n", activePreset_);

  for (int p = 0; p < kPresetCount; ++p) {
    char key[24];
    
    // name
    snprintf(key, sizeof(key), "p%d_name", p);
    String nm = prefs_.getString(key, String("Preset ") + String(p + 1));
    strncpy(presets_[p].name, nm.c_str(), sizeof(presets_[p].name));
    presets_[p].name[sizeof(presets_[p].name) - 1] = '\0';
    Serial.printf("  Preset %d name: '%s'\n", p, presets_[p].name);

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
          Serial.printf("    P%d Ch%d Slot%d: en=%d tp=%.2f st=%d co=%d\n",
                        p, c, s, A.enabled, A.tripPoint, (int)A.style, (int)A.color);
        }

        // runtime cleared
        A.tripped = false;
        A.firedOnce = false;
        A.trippedAtMs = 0;
        A.silencedUntilMs = 0;
      }
    }
  }

  refreshAlarmUIFromPreset(getActivePreset());

  prefs_.end();
  Serial.println("[AlarmManager] prefs loaded successfully");
}

void AlarmManager::savePrefs() {
  if (!prefs_.begin("m4alarms", false)) {
    Serial.println("[AlarmManager] prefs save begin failed");
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
  Serial.println("[AlarmManager] prefs saved");
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

void AlarmManager::setEnabled(uint8_t preset, AlarmChannel ch, AlarmSlot sl, bool en) {
  if (preset >= kPresetCount) preset = 0;
  presets_[preset].alarms[(int)ch][(int)sl].enabled = en;
  savePrefs();
}

void AlarmManager::setTripPoint(uint8_t preset, AlarmChannel ch, AlarmSlot sl, float baseValue) {
  Serial.println("[AlarmManager] setTripPoint called");
  if (preset >= kPresetCount) preset = 0;
  float v = baseValue;
  clampAndValidate(ch, v);
  presets_[preset].alarms[(int)ch][(int)sl].tripPoint = v;
  savePrefs();
  Serial.printf("[AlarmManager] Trip point set to %.2f\n", v);
}

void AlarmManager::setStyle(uint8_t preset, AlarmChannel ch, AlarmSlot sl, AlarmStyle st) {
  if (preset >= kPresetCount) preset = 0;
  presets_[preset].alarms[(int)ch][(int)sl].style = st;
  // Reset runtime flags on style change
  AlarmConfig& A = presets_[preset].alarms[(int)ch][(int)sl];
  A.tripped = false; A.firedOnce = false; A.trippedAtMs = 0;
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
  // TODO: call this during STAGED and PULLING only.
  // Pull current values from StateManager
  // float dFt = StateManager::state().distanceInFeet;
  // float mph = StateManager::state().speedInMPH;
  // float rpm = StateManager::state().rpm;
  // For each enabled alarm in active preset:
  //   apply style logic
  //   set A.tripped, update A.firedOnce, timestamps
  //   emit events to horn, relays, UI as needed
  // TODO: hook horn logic using buzzerSilencedUntilMs_
}

void AlarmManager::resetForStateEntry(StateManager& /*sm*/) {
  // TODO: call on STAGED entry
  AlarmPreset& P = presets_[activePreset_];
  for (int c = 0; c < kChannels; ++c) {
    for (int s = 0; s < kSlots; ++s) {
      AlarmConfig& A = P.alarms[c][s];
      A.tripped = false;
      A.firedOnce = false;
      A.trippedAtMs = 0;
      A.silencedUntilMs = 0;
    }
  }
  buzzerSilencedUntilMs_ = 0;
}

// ----------------------------
// Horn and silence
// ----------------------------
void AlarmManager::silenceForMs(uint32_t ms) {
  buzzerSilencedUntilMs_ = millis() + ms;
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

  Serial.println("[AlarmManager] Warning: no preset button checked!");

  // fallback: none checked, return last known preset
  return AlarmManager::getActivePreset();
}

void AlarmManager::handleAlarmChange(AlarmChannel ch, AlarmSlot sl, AlarmField field, lv_event_t* e) {
  const uint8_t preset = (uint8_t)getActivePresetFromButtons();
  const lv_event_code_t code = lv_event_get_code(e);
  if (isAlarmUIRefreshing()) return;

  switch (field) {
    case AlarmField::ENABLE:
    case AlarmField::STYLE:
    case AlarmField::COLOR:
      if (code != LV_EVENT_VALUE_CHANGED) return;
      break;
    case AlarmField::TRIPPOINT:
      if (code != LV_EVENT_DEFOCUSED && code != LV_EVENT_READY) return;
      break;
  }

  lv_obj_t* target = lv_event_get_target(e);
  if (!target) return;

  switch (field) {
    case AlarmField::ENABLE: {
      bool en = lv_obj_has_state(target, LV_STATE_CHECKED);
      setEnabled(preset, ch, sl, en);
    } break;

    case AlarmField::TRIPPOINT: {
      const char* txt = lv_textarea_get_text(target);
      if (!txt) return;
      // parse
      char* endp = nullptr;
      float v = strtof(txt, &endp);
      if (endp == txt || isnan(v)) {
        // TODO: Implement invalid input handling (eg. less than 0, greater than track length for distance, non-numeric)
        // TODO: show invalid modal
        // refresh UI to restore old value
        refreshAlarmUIFromPreset(preset);
        return;
      }
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
