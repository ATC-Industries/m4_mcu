// src/JudgeModule.cpp
#include "JudgeModule.h"

#include "Arduino.h"
#include "ArduinoJson.h"
#include "AlarmManager.h"
#include "M4MessageStruct.h"
#include "M4CommsHelpers.h"
#include "StateManager.h"

#define LOG_TAG "JudgeModule"
#define LOG_DEBUG_DISABLE 1
#include "Logging.h"

#include "ui/ui.h"
#include "ScreenUpdater.h"

extern esp_err_t sendMessage(const uint8_t *peerAddress,
                             msg_type messageType,
                             action_type messageAction,
                             String payload,
                             priority messagePriority);


namespace {
  bool s_judge_mode_active = false;
  uint8_t s_tracked_host_id = 0;  // 0 means "accept any host" for now

  JudgeModule::HostSnapshot s_last_snapshot = {
      0,
      PullState::READY,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      0.0f,
  };

  constexpr uint32_t kJudgeValuesEveryMs = 200;  // 5 Hz
  constexpr uint32_t kJudgeConfigEveryMs = 1000;
  constexpr uint32_t kJudgeHistoryEveryMs = 700;
  uint32_t s_last_judge_values_ms = 0;
  uint32_t s_last_judge_config_ms = 0;
  uint32_t s_last_judge_history_ms = 0;
  int s_history_broadcast_index = 0;
  int s_display_host_unit_id = 0;
  String s_display_driver_name;
  int s_display_driver_number = 0;
  String s_display_class_name;

  bool pullResultEquals(const PullResult& lhs, const PullResult& rhs) {
    return lhs.driverName == rhs.driverName &&
           lhs.driverNumber == rhs.driverNumber &&
           lhs.className == rhs.className &&
           lhs.classWeight == rhs.classWeight &&
           fabs(lhs.maxSpeedMPH - rhs.maxSpeedMPH) < 0.001f &&
           fabs(lhs.maxDistanceFeet - rhs.maxDistanceFeet) < 0.001f &&
           fabs(lhs.maxRPM - rhs.maxRPM) < 0.001f &&
           lhs.timestamp == rhs.timestamp;
  }

  void setButtonEnabled(lv_obj_t* obj, bool enabled) {
    if (!obj) return;
    if (enabled) {
      lv_obj_clear_state(obj, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(obj, LV_STATE_DISABLED);
    }
  }

  void setContainerClickable(lv_obj_t* obj, bool clickable) {
    if (!obj) return;
    if (clickable) {
      lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    }
  }

  void sendJudgeConfigBroadcast(int hostId) {
    AlarmConfig configs[kChannels][kSlots];
    for (int c = 0; c < kChannels; ++c) {
      for (int s = 0; s < kSlots; ++s) {
        configs[c][s] = AlarmManager::getConfigActive(static_cast<AlarmChannel>(c),
                                                      static_cast<AlarmSlot>(s));
      }
    }

    JsonDocument doc;
    doc["action"] = SEND_JUDGE_DATA;
    doc["k"] = "c";
    doc["h"] = hostId;
    doc["u"] = static_cast<uint8_t>(StateManager::prefs().unitSystem);
    doc["tl"] = StateManager::prefs().trackLengthFeet;
    doc["ap"] = AlarmManager::getActivePreset();

    JsonArray alarms = doc["a"].to<JsonArray>();
    for (int c = 0; c < kChannels; ++c) {
      for (int s = 0; s < kSlots; ++s) {
        const AlarmConfig& cfg = configs[c][s];
        JsonArray row = alarms.add<JsonArray>();
        row.add(cfg.enabled ? 1 : 0);
        row.add(cfg.tripPoint);
        row.add(static_cast<uint8_t>(cfg.style));
        row.add(static_cast<uint8_t>(cfg.color));
      }
    }

    String payload;
    serializeJson(doc, payload);
    sendMessage(kBroadcastAddress, BROADCAST, SEND_JUDGE_DATA, payload, MEDIUM_PRIORITY);
  }

  void sendJudgeMetaBroadcast(int hostId) {
    JsonDocument doc;
    doc["action"] = SEND_JUDGE_DATA;
    doc["k"] = "m";
    doc["h"] = hostId;
    doc["mid"] = hostId;
    doc["dn"] = StateManager::prefs().driverNumber;
    doc["d"] = StateManager::prefs().driverName;
    doc["cn"] = StateManager::prefs().pullingClassName;

    String payload;
    serializeJson(doc, payload);
    sendMessage(kBroadcastAddress, BROADCAST, SEND_JUDGE_DATA, payload, MEDIUM_PRIORITY);
  }

  void sendJudgeHistoryBroadcast(int hostId) {
    const int count = StateManager::getPullHistoryCount();

    JsonDocument doc;
    doc["action"] = SEND_JUDGE_DATA;
    doc["k"] = "r";
    doc["h"] = hostId;
    doc["c"] = count;

    if (count <= 0) {
      String payload;
      serializeJson(doc, payload);
      sendMessage(kBroadcastAddress, BROADCAST, SEND_JUDGE_DATA, payload, LOW_PRIORITY);
      return;
    }

    if (s_history_broadcast_index >= count) {
      s_history_broadcast_index = 0;
    }

    const PullResult* pull = StateManager::getPullResult(s_history_broadcast_index);
    if (!pull) {
      s_history_broadcast_index = 0;
      return;
    }

    doc["i"] = s_history_broadcast_index;
    doc["dn"] = pull->driverNumber;
    doc["d"] = pull->driverName;
    doc["cn"] = pull->className;
    doc["cw"] = pull->classWeight;
    doc["s"] = pull->maxSpeedMPH;
    doc["df"] = pull->maxDistanceFeet;
    doc["r"] = pull->maxRPM;
    doc["t"] = pull->timestamp;

    String payload;
    serializeJson(doc, payload);
    sendMessage(kBroadcastAddress, BROADCAST, SEND_JUDGE_DATA, payload, LOW_PRIORITY);

    s_history_broadcast_index = (s_history_broadcast_index + 1) % count;
  }

  void applyJudgeModeToAllScreens() {
    JudgeModule::applyJudgeModeToMainScreen();
    JudgeModule::applyJudgeModeToSettingsScreen();
    JudgeModule::applyJudgeModeToPullHistoryScreen();
  }

}  // namespace

namespace JudgeModule {

void begin() {
  s_judge_mode_active = StateManager::getJudgeMode();
  s_tracked_host_id = static_cast<uint8_t>(StateManager::getHostM4ID());

  LOGI("begin judgeMode=%d trackedHost=%u",
       s_judge_mode_active ? 1 : 0,
       s_tracked_host_id);

  if (s_judge_mode_active) {
    applyJudgeModeToAllScreens();
  }
}

void tick() {
  bool judge = StateManager::getJudgeMode();

  // Judge unit should NOT broadcast, it only listens
  if (judge) {
    return;
  }

  // Only sled units reach here

  uint32_t now = millis();
  if (now - s_last_judge_values_ms < kJudgeValuesEveryMs) {
    return;
  }
  s_last_judge_values_ms = now;

  // Pull all values once so logs and payload always match
  const int       hostId        = StateManager::getM4ID();
  const PullState pullState     = StateManager::getPullState();
  const SystemState& systemState = StateManager::state();
  const float     curDistFeet   = systemState.distanceInFeet;
  const float     curSpeedMph   = systemState.speedInMPH;
  const float     curRpm        = systemState.rpm;
  const float     maxDistFeet   = systemState.maxDistanceInFeet;
  const float     maxSpeedMph   = systemState.maxSpeedInMPH;
  const float     maxRpm        = systemState.maxRpm;
  const uint8_t   activePreset  = AlarmManager::getActivePreset();
  const uint8_t   tripMask      = AlarmManager::getActiveTripMask();

  JsonDocument doc;
  doc["action"]      = SEND_JUDGE_DATA;
  doc["k"]           = "t";
  doc["h"]           = hostId;
  doc["ps"]          = static_cast<int>(pullState);

  // Live values
  doc["d"]           = curDistFeet;
  doc["s"]           = curSpeedMph;
  doc["r"]           = curRpm;

  // Max values
  doc["md"]          = maxDistFeet;
  doc["ms"]          = maxSpeedMph;
  doc["mr"]          = maxRpm;
  doc["ap"]          = activePreset;
  doc["tm"]          = tripMask;

  String payload;
  serializeJson(doc, payload);

  LOGD("Broadcast judge data host=%d state=%d "
       "dist=%.2f speed=%.2f rpm=%.1f maxDist=%.2f maxSpeed=%.2f maxRpm=%.1f",
       hostId,
       static_cast<int>(pullState),
       curDistFeet, curSpeedMph, curRpm,
       maxDistFeet, maxSpeedMph, maxRpm);

  sendMessage(kBroadcastAddress,
              BROADCAST,
              SEND_JUDGE_DATA,
              payload,
              HIGH_PRIORITY);

  if (now - s_last_judge_config_ms >= kJudgeConfigEveryMs) {
    s_last_judge_config_ms = now;
    sendJudgeConfigBroadcast(hostId);
    sendJudgeMetaBroadcast(hostId);
  }

  if (now - s_last_judge_history_ms >= kJudgeHistoryEveryMs) {
    s_last_judge_history_ms = now;
    sendJudgeHistoryBroadcast(hostId);
  }
}


void onJudgeModeChanged(bool enabled) {
  if (s_judge_mode_active == enabled) {
    return;
  }

  s_judge_mode_active = enabled;
  LOGI("judge mode toggled -> %d", enabled ? 1 : 0);

  if (!enabled) {
    StateManager::clearJudgeMirrorUnits();
    AlarmManager::loadPrefs();
    s_display_host_unit_id = 0;
    s_display_driver_name = "";
    s_display_driver_number = 0;
    s_display_class_name = "";
  }

  applyJudgeModeToAllScreens();
}

bool isJudgeModeActive() {
  return s_judge_mode_active;
}

void setTrackedHostId(uint8_t host_id) {
  s_tracked_host_id = host_id;
  LOGI("set tracked host id -> %u", host_id);
}

uint8_t getTrackedHostId() {
  return s_tracked_host_id;
}

void onHostStatusBroadcast(const HostSnapshot& snap) {
  LOGD("Host %d state=%d dist=%.2f speed=%.2f rpm=%.1f "
       "maxDist=%.2f maxSpeed=%.2f maxRpm=%.1f",
       snap.host_id,
       static_cast<int>(snap.pull_state),
       snap.distanceFeet, snap.speedMph, snap.rpm,
       snap.maxDistanceFeet, snap.maxSpeedMph, snap.maxRpm);

  s_last_snapshot = snap;

  // 1) Mirror pull state into StateManager so updateMainScreen()
  //    can decide when to show MAX labels.
  SystemState& state = StateManager::state();
  state.currentPullState = snap.pull_state;
  state.distanceInFeet = snap.distanceFeet;
  state.speedInMPH = snap.speedMph;
  state.rpm = snap.rpm;
  state.maxDistanceInFeet = snap.maxDistanceFeet;
  state.maxSpeedInMPH = snap.maxSpeedMph;
  state.maxRpm = snap.maxRpm;

  PullStateManager::updateUIForState(snap.pull_state);
}

void applyRemoteConfig(UnitSystem unitSystem,
                       float trackLengthFeet,
                       uint8_t activePreset,
                       const AlarmConfig configs[kChannels][kSlots]) {
  if (!s_judge_mode_active) {
    return;
  }

  StateManager::setJudgeMirrorUnits(unitSystem, trackLengthFeet);
  AlarmManager::applyMirrorConfig(activePreset, configs);
}

void applyRemoteMeta(int hostUnitId,
                     const String& driverName,
                     int driverNumber,
                     const String& className) {
  if (!s_judge_mode_active) {
    return;
  }

  s_display_host_unit_id = hostUnitId;
  s_display_driver_name = driverName;
  s_display_driver_number = driverNumber;
  s_display_class_name = className;
}


void applyRemotePullHistory(const PullResult* pulls, int count) {
  if (!s_judge_mode_active || !pulls || count <= 0) {
    return;
  }

  int capped = count;
  if (capped > MAX_PULL_HISTORY) {
    capped = MAX_PULL_HISTORY;
  }

  // Write into preferences via StateManager
  SystemPreferences& prefs = StateManager::prefs();
  prefs.pullHistoryCount = capped;

  for (int i = 0; i < capped; ++i) {
    prefs.pullHistory[i] = pulls[i];
  }

  StateManager::savePreferences();

  LOGI("pull history synced from host, count=%d", capped);
}

void applyRemotePullHistoryRow(int index, int totalCount, const PullResult& pull) {
  if (!s_judge_mode_active || totalCount < 0) {
    return;
  }

  SystemPreferences& prefs = StateManager::prefs();

  if (totalCount == 0) {
    bool changed = prefs.pullHistoryCount != 0;
    prefs.pullHistoryCount = 0;
    for (int i = 0; i < MAX_PULL_HISTORY; ++i) {
      if (!pullResultEquals(prefs.pullHistory[i], PullResult{})) {
        changed = true;
      }
      prefs.pullHistory[i] = PullResult{};
    }
    if (changed) {
      StateManager::savePreferences();
    }
    return;
  }

  if (index < 0 || index >= totalCount || index >= MAX_PULL_HISTORY) {
    return;
  }

  const int cappedTotal = totalCount > MAX_PULL_HISTORY ? MAX_PULL_HISTORY : totalCount;
  bool changed = prefs.pullHistoryCount != cappedTotal ||
                 !pullResultEquals(prefs.pullHistory[index], pull);

  prefs.pullHistoryCount = cappedTotal;
  prefs.pullHistory[index] = pull;

  if (changed && index == cappedTotal - 1) {
    StateManager::savePreferences();
  }
}

const HostSnapshot& getLastSnapshot() {
  return s_last_snapshot;
}

int getDisplayHostUnitId() {
  return s_display_host_unit_id;
}

const char* getDisplayDriverName() {
  return s_display_driver_name.c_str();
}

int getDisplayDriverNumber() {
  return s_display_driver_number;
}

const char* getDisplayClassName() {
  return s_display_class_name.c_str();
}

// UI helpers - stubbed so you can drop in the LVGL calls

void applyJudgeModeToMainScreen() {
  bool judge = StateManager::prefs().isJudgeMode;

  // Main state buttons
  setButtonEnabled(uic_MainButtonREADYStage,     !judge);
  setButtonEnabled(uic_MainButtonSTAGEDCancel,   !judge);
  setButtonEnabled(uic_MainButtonPULLINGStop,    !judge);
  setButtonEnabled(uic_MainButtonPULLINGStop1,   !judge);
  setButtonEnabled(uic_MainButtonPULLENDDiscard, !judge);
  setButtonEnabled(uic_MainButtonPULLENDSave,    !judge);

  // Alarm preset container that fires MainScreenPresetButtonClicked()
  setContainerClickable(uic_MainContainerAlarmPresetContainer, !judge);

  //LOGD("applyJudgeModeToMainScreen judge=%d", judge ? 1 : 0);
}



void applyJudgeModeToSettingsScreen() {
  bool judge = StateManager::prefs().isJudgeMode;

  // Sync the Judge Stand toggle itself
  if (uic_Settings1SwitchUseJudgeSwitch) {
    if (judge) {
      lv_obj_add_state(uic_Settings1SwitchUseJudgeSwitch, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(uic_Settings1SwitchUseJudgeSwitch, LV_STATE_CHECKED);
    }
  }

  lv_obj_t* tabview = ui_Settings1TabviewSettingsView;
  if (!tabview) {
    return;
  }

  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabview);

  // Same list you already had
  const int disable_tabs[] = {1, 2, 3, 4, 6};
  const int disable_count = sizeof(disable_tabs) / sizeof(disable_tabs[0]);

  if (judge) {
    LOGI("Settings screen: judge mode active - disabling certain settings");

    // Disable various settings controls
    lv_obj_add_state(ui_Settings1SwitchUnitsToggle, LV_STATE_DISABLED);
    lv_obj_add_state(uic_M4IDNumberTitleTextArea, LV_STATE_DISABLED);
    lv_obj_add_state(uic_Settings1TextareaTrackLengthText, LV_STATE_DISABLED);
    lv_obj_add_state(ui_Settings1TextareaCalibrationNumberTextArea, LV_STATE_DISABLED);
    lv_obj_add_state(uic_Settings1SwitchTachToggle, LV_STATE_DISABLED);
    lv_obj_add_state(uic_Settings1SwitchLimitToggle, LV_STATE_DISABLED);
    lv_obj_add_state(uic_Settings1SwitchRelaysToggle, LV_STATE_DISABLED);

    // Disable selected tabs
    for (int i = 0; i < disable_count; i++) {
      lv_btnmatrix_set_btn_ctrl(tab_btns, disable_tabs[i], LV_BTNMATRIX_CTRL_DISABLED);
    }

  } else {
    LOGI("Settings screen: sled pull mode active - enabling all settings");

    // Enable various settings controls
    lv_obj_clear_state(ui_Settings1SwitchUnitsToggle, LV_STATE_DISABLED);
    lv_obj_clear_state(uic_M4IDNumberTitleTextArea, LV_STATE_DISABLED);
    lv_obj_clear_state(uic_Settings1TextareaTrackLengthText, LV_STATE_DISABLED);
    lv_obj_clear_state(ui_Settings1TextareaCalibrationNumberTextArea, LV_STATE_DISABLED);
    lv_obj_clear_state(uic_Settings1SwitchTachToggle, LV_STATE_DISABLED);
    lv_obj_clear_state(uic_Settings1SwitchLimitToggle, LV_STATE_DISABLED);
    lv_obj_clear_state(uic_Settings1SwitchRelaysToggle, LV_STATE_DISABLED);

    // Enable tabs again
    for (int i = 0; i < disable_count; i++) {
      lv_btnmatrix_clear_btn_ctrl(tab_btns, disable_tabs[i], LV_BTNMATRIX_CTRL_DISABLED);
    }
  }

  if (judge) {
    lv_obj_add_flag(uic_Settings1PanelSettingJudgeMCUPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_Settings1PanelSettingJudgeJudgePanel, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(uic_Settings1PanelSettingJudgeJudgePanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(uic_Settings1PanelSettingJudgeMCUPanel, LV_OBJ_FLAG_HIDDEN);
  }
  
  LOGD("applyJudgeModeToSettingsScreen judge=%d", judge ? 1 : 0);
}


void applyJudgeModeToPullHistoryScreen() {
  bool judge = s_judge_mode_active;

  // Example - hide "clear history" or make it disabled in judge mode
  if (uic_PullHistoryScreenButtondeletePullHistoryButton1) {
    if (judge) {
      lv_obj_add_state(uic_PullHistoryScreenButtondeletePullHistoryButton1, LV_STATE_DISABLED);
    } else {
      lv_obj_clear_state(uic_PullHistoryScreenButtondeletePullHistoryButton1, LV_STATE_DISABLED);
    }
  }
}

}  // namespace JudgeModule
