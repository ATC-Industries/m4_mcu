// src/JudgeModule.cpp
#include "JudgeModule.h"

#include "Arduino.h"
#include "ArduinoJson.h"
#include "M4MessageStruct.h"
#include "M4CommsHelpers.h"
#include "StateManager.h"
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
  uint32_t s_last_judge_values_ms = 0;

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

  void pushSnapshotIntoStateManager(const JudgeModule::HostSnapshot& s) {
    if (!s_judge_mode_active) {
      return;
    }

    // Set pull state without triggering relays or SpeedModule
    StateManager::setPullState(s.pull_state);

    // Use raw host values so max tracking behaves same as host
    StateManager::setRPM(s.rpm);
    StateManager::setSpeed(s.speedMph);
    StateManager::setDistance(s.distanceFeet);

    // Max values - if host is sending true max, these are redundant but safe
    // to call while state is PULLING. They handle their own max logic.
    if (s.pull_state == PullState::PULLING || s.pull_state == PullState::PULLEND) {
      // One way: set current equal to host max at end of pull
      // This will naturally set max fields correctly because they exceed previous.
      StateManager::setRPM(s.maxRpm);
      StateManager::setSpeed(s.maxSpeedMph);
      StateManager::setDistance(s.maxDistanceFeet);
    }

    // Make sure the UI containers match the remote pull state
    PullStateManager::updateUIForState(s.pull_state);
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

  LOGI("[JudgeModule] begin judgeMode=%d trackedHost=%u",
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

  JsonDocument doc;
  doc["action"]    = SEND_JUDGE_DATA;
  doc["host"]      = StateManager::getM4ID();

  PullState ps = StateManager::getPullState();
  doc["pullState"] = static_cast<int>(ps);

  const SystemState& st = StateManager::state();
  doc["distance"]    = st.distanceInFeet;
  doc["speed"]       = st.speedInMPH;
  doc["rpm"]         = st.rpm;
  doc["maxDistance"] = st.maxDistanceInFeet;
  doc["maxSpeed"]    = st.maxSpeedInMPH;
  doc["maxRpm"]      = st.maxRpm;

  String payload;
  serializeJson(doc, payload);

  LOGD("[JudgeModule] Broadcast judge data host=%d state=%d dist=%.2f speed=%.2f rpm=%.1f",
       StateManager::getM4ID(),
       static_cast<int>(ps),
       st.distanceInFeet,
       st.speedInMPH,
       st.rpm);

  sendMessage(kBroadcastAddress,
              BROADCAST,
              SEND_JUDGE_DATA,
              payload,
              HIGH_PRIORITY);
}


void onJudgeModeChanged(bool enabled) {
  if (s_judge_mode_active == enabled) {
    return;
  }

  s_judge_mode_active = enabled;
  LOGI("[JudgeModule] judge mode toggled -> %d", enabled ? 1 : 0);

  if (!enabled) {
    // Leaving judge mode
    // At some point we might want to clear any host specific state here.
  }

  applyJudgeModeToAllScreens();
}

bool isJudgeModeActive() {
  return s_judge_mode_active;
}

void setTrackedHostId(uint8_t host_id) {
  s_tracked_host_id = host_id;
  LOGI("[JudgeModule] set tracked host id -> %u", host_id);
}

uint8_t getTrackedHostId() {
  return s_tracked_host_id;
}

void onHostStatusBroadcast(const HostSnapshot& snapshot) {
  if (!s_judge_mode_active) {
    return;
  }

  if (s_tracked_host_id != 0 && snapshot.host_id != s_tracked_host_id) {
    return;
  }

  s_last_snapshot = snapshot;

  LOGD("[JudgeModule] Host %u state=%d dist=%.2f speed=%.2f rpm=%.1f",
       snapshot.host_id,
       static_cast<int>(snapshot.pull_state),
       snapshot.distanceFeet,
       snapshot.speedMph,
       snapshot.rpm);

  pushSnapshotIntoStateManager(snapshot);
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

  LOGI("[JudgeModule] pull history synced from host, count=%d", capped);
}

const HostSnapshot& getLastSnapshot() {
  return s_last_snapshot;
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

  //LOGD("[JudgeModule] applyJudgeModeToMainScreen judge=%d", judge ? 1 : 0);
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
    Serial.println("[SettingsScreen] Judge Mode active - disabling certain settings");

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
    Serial.println("[SettingsScreen] Sled Pull Mode active - enabling all settings");

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
  
  LOGD("[JudgeModule] applyJudgeModeToSettingsScreen judge=%d", judge ? 1 : 0);
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
