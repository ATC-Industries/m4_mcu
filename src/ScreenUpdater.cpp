#include "ScreenUpdater.h"

#include <cstring>

#include "../ui/ui.h"
#include "../ui/ui_themes.h"
#include "AlarmManager.h"
#define LOG_TAG "ScreenUpdater"
#include "Logging.h"
#include "PullStateManager.h"
#include "StateManager.h"
#include "TachClient.h"
#include "remotes/RemoteManager.h"
#include "JudgeModule.h"

// Last known values to avoid unnecessary redraws
static float lastDisplayedSpeed = -1.0f;
static float lastDisplayedDistance = -1.0f;
static float lastDisplayedRPM = -1.0f;
static std::string lastDisplayedClassName;
static std::string lastDisplayedDriverName;
static int lastDisplayedDriverNumber = -1;
static int lastDisplayedUnitID = -1;
static uint8_t lastDisplayedPresetIdx = 0xFF;
static int lastDisplayedTrackLength = -1;
static int lastDisplayedProgressDistance = -1;
static bool lastDisplayedIsMetric = false;
static bool lastDisplayedShowMax = false;
static bool lastDisplayedDistanceTitleVisible = false;
static bool lastDisplayedTssConnected = false;
static bool lastDisplayedTachVisible = false;
static bool lastDisplayedLimitVisible = false;
static bool lastDisplayedRelayVisible = false;
static bool lastDisplayedSilenceAlarmVisible = false;
static bool alarmUiRefreshing = false;

static const lv_color_t kPresetColors[] = {
    lv_color_hex(0xA70909),  // Preset 1 - Red
    lv_color_hex(0xF06B00),  // Preset 2 - Orange
    lv_color_hex(0x1FA709),  // Preset 3 - Green
    lv_color_hex(0x346DE1)   // Preset 4 - Blue
};

void primeMainScreenValues() {
  lastDisplayedSpeed = NAN;
  lastDisplayedDistance = NAN;
  lastDisplayedRPM = NAN;
  lastDisplayedTrackLength = -1;
  lastDisplayedProgressDistance = -1;
  lastDisplayedUnitID = -1;
  lastDisplayedPresetIdx = 0xFF;
  lastDisplayedIsMetric = false;
  lastDisplayedShowMax = false;
  lastDisplayedDistanceTitleVisible = false;
  lastDisplayedTssConnected = false;
  lastDisplayedTachVisible = false;
  lastDisplayedLimitVisible = false;
  lastDisplayedRelayVisible = false;
  lastDisplayedSilenceAlarmVisible = false;
}

// visibility helper
static inline void set_visible(lv_obj_t* obj, bool on) {
  if (!obj) return;
  if (on) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

extern VERSION pairedTractorVersion;  // defined in M4CommsHelpers.cpp

static inline void macToStr(const uint8_t* mac, char out[18]) {
  if (!mac) { strcpy(out, "--"); return; }
  snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
static inline bool macIsEmpty(const uint8_t* mac) {
  if (!mac) return true;
  for (int i = 0; i < 6; ++i) if (mac[i] != 0) return false;
  return true;
}



static lv_color_t alarmColorToLv(AlarmColor color) {
  switch (color) {
    case AlarmColor::RED:
      return COLOR_INDIC_RED;
    case AlarmColor::GREEN:
      return COLOR_INDIC_GREEN;
    case AlarmColor::YELLOW:
    default:
      return COLOR_INDIC_YELLOW;
  }
}

static void applyDistanceAlarmColor(lv_obj_t* bar, AlarmColor color) {
  if (!bar) return;
  lv_color_t col = alarmColorToLv(color);

  // Make sure MAIN outline is not coloring the whole bar
  lv_obj_set_style_outline_opa(bar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

  // Draw just the thin line on the INDICATOR
  lv_obj_set_style_bg_opa(bar, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_border_color(bar, col, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(bar, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, LV_PART_INDICATOR | LV_STATE_DEFAULT);
}

static void applyDistanceAlarmColorToLabel(lv_obj_t* label, AlarmColor color) {
  if (!label) return;
  lv_color_t col = alarmColorToLv(color);
  // Changes background color to col
  lv_obj_set_style_bg_color(label, col, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  // Changes text color too col
  // lv_obj_set_style_text_color(label, col, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (color == AlarmColor::YELLOW) {
    // for yellow background, use black text for contrast
    lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else {
    // for red/green background, use white text for contrast
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  }
}

static inline void set_bar_visible(lv_obj_t* bar, bool visible);

static void setDistanceAlarmTripState(lv_obj_t* bar, lv_obj_t* label, AlarmColor color,
                                      bool enabled, bool tripped) {
  if (bar) {
    set_bar_visible(bar, enabled);
    if (enabled) {
      applyDistanceAlarmColor(bar, color);
      lv_obj_set_style_border_opa(bar, tripped ? 255 : 96, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
  }

  if (label) {
    if (enabled) {
      lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
      applyDistanceAlarmColorToLabel(label, color);
      lv_obj_set_style_bg_opa(label, tripped ? LV_OPA_COVER : 96, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_text_opa(label, tripped ? LV_OPA_COVER : 180, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
  }
}


bool isAlarmUIRefreshing() { return alarmUiRefreshing; }

static void setIndicatorColor(lv_obj_t* obj, uint8_t themeID, uint8_t opacity = 255) {
  if (!obj) return;
  
  lv_color_t color = COLOR_INDIC_DISABLED;

  switch (themeID) {
    case UI_THEME_COLOR_INDICGREEN:
      color = COLOR_INDIC_GREEN;
      break;
    case UI_THEME_COLOR_INDICRED:
      color = COLOR_INDIC_RED;
      break;
    case UI_THEME_COLOR_INDICYELLOW:
      color = COLOR_INDIC_YELLOW;
      break;
    case UI_THEME_COLOR_INDICDISABLED:
      color = COLOR_INDIC_DISABLED;
      break;
  }

  lv_obj_set_style_bg_color(obj, color, 0);
  lv_obj_set_style_bg_opa(obj, opacity, 0);  // NEW LINE - sets opacity
}

void SetupJudgeSwitchHitArea() {
  lv_obj_t* sw = uic_Settings1SwitchUseJudgeSwitch;

  // Add 12 px clickable padding on all sides if available in your 8.3 build
  #if LVGL_VERSION_MAJOR == 8
    lv_obj_set_ext_click_area(sw, 12);
  #endif

  // Make sure events do not bubble upward
  lv_obj_clear_flag(sw, LV_OBJ_FLAG_EVENT_BUBBLE);

  // If the parent is scrollable, it can steal tiny drags. Turn it off here if you can.
  lv_obj_t* parent = lv_obj_get_parent(sw);
  lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);  // or keep scroll and raise thresholds elsewhere
}

void updateSettingsScreen() {
    // Set units toggle to match current preference
    (StateManager::getUnitSystem() == UnitSystem::METRIC)
        ? lv_obj_add_state(ui_Settings1SwitchUnitsToggle, LV_STATE_CHECKED)
        : lv_obj_clear_state(ui_Settings1SwitchUnitsToggle, LV_STATE_CHECKED);
    if (ui_Settings1LabelTrackLengthUnitsTitle) {
      lv_label_set_text(ui_Settings1LabelTrackLengthUnitsTitle,
                        StateManager::getUnitSystem() == UnitSystem::METRIC ? "Meters" : "Feet");
    }
    // Toggle switches
    StateManager::getTachEnabled() ? lv_obj_add_state(uic_Settings1SwitchTachToggle, LV_STATE_CHECKED)
                                   : lv_obj_clear_state(uic_Settings1SwitchTachToggle, LV_STATE_CHECKED);

    StateManager::getLimitSwitchesEnabled() ? lv_obj_add_state(uic_Settings1SwitchLimitToggle, LV_STATE_CHECKED)
                                            : lv_obj_clear_state(uic_Settings1SwitchLimitToggle, LV_STATE_CHECKED);

    StateManager::getRelaysEnabled() ? lv_obj_add_state(uic_Settings1SwitchRelaysToggle, LV_STATE_CHECKED)
                                     : lv_obj_clear_state(uic_Settings1SwitchRelaysToggle, LV_STATE_CHECKED);

    StateManager::prefs().screenRotation180 ? lv_obj_add_state(ui_Settings1SwitchRotateScreenToggle, LV_STATE_CHECKED)
                                            : lv_obj_clear_state(ui_Settings1SwitchRotateScreenToggle, LV_STATE_CHECKED);

    // Brightness
    lv_slider_set_value(uic_Settings1SliderBrightnessSlider, StateManager::prefs().screenBrightness, LV_ANIM_OFF);
    if (uic_Settings1LabelBrightness) {
      lv_label_set_text_fmt(uic_Settings1LabelBrightness, "%d%%",
                            (StateManager::prefs().screenBrightness * 100) / 255);
    }

    // M4 ID Number
    char M4IdBuf[16];
    snprintf(M4IdBuf, sizeof(M4IdBuf), "%d", StateManager::prefs().M4IDNumber);
    lv_textarea_set_text(uic_M4IDNumberTitleTextArea, M4IdBuf);

    // Increase target of judge toggle
    SetupJudgeSwitchHitArea();


    // Track length
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", StateManager::getTrackLength());
    lv_textarea_set_text(uic_Settings1TextareaTrackLengthText, buf);

    char hostBuf[16];
    snprintf(hostBuf, sizeof(hostBuf), "%d", StateManager::prefs().HostM4IDNumber);
    if (uic_Settings1TextareaHostMCUID1) {
      lv_textarea_set_text(uic_Settings1TextareaHostMCUID1, hostBuf);
    }
 
  // Judge Mode Switch
  bool isJudgeMode = StateManager::prefs().isJudgeMode;
  if (uic_Settings1SwitchUseJudgeSwitch) {
    if (isJudgeMode) {
      lv_obj_add_state(uic_Settings1SwitchUseJudgeSwitch, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(uic_Settings1SwitchUseJudgeSwitch, LV_STATE_CHECKED);
    }
  }

  bool isAutoConnect = StateManager::getIsAutoConnectTractor();
  if (uic_Settings1SwitchTachAutoConnectToggle) {
    if (isAutoConnect) {
      lv_obj_add_state(uic_Settings1SwitchTachAutoConnectToggle, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(uic_Settings1SwitchTachAutoConnectToggle, LV_STATE_CHECKED);
    }
  }
  // TSS status line
  {
    bool tssConnected = Tach::state.isTSSConnected;
    int  rssi = Tach::state.lastRssi;
    const uint8_t* mac = StateManager::getPairedTractorAddress();

    // 1) Status
    if (uic_Settings1LabelTSSPairStatus) {
      if (tssConnected) {
        // Show simple connected text with RSSI
        char buf[48];
        snprintf(buf, sizeof(buf), "Connected  RSSI %d dBm", rssi);
        lv_label_set_text(uic_Settings1LabelTSSPairStatus, buf);
      } else {
        lv_label_set_text(uic_Settings1LabelTSSPairStatus, "Not connected");
      }
    }

    // 2) Version
    if (uic_Settings1LabelTSSPairVersion) {
      char ver[32];
      if (pairedTractorVersion.major == 0 && pairedTractorVersion.minor == 0 && pairedTractorVersion.patch == 0) {
        strcpy(ver, "--");
      } else {
        snprintf(ver, sizeof(ver), "%d.%d.%d",
                pairedTractorVersion.major,
                pairedTractorVersion.minor,
                pairedTractorVersion.patch);
      }
      lv_label_set_text(uic_Settings1LabelTSSPairVersion, ver);
    }

    // 3) MAC
    if (uic_Settings1LabelTSSPairMAC) {
      char macStr[18];
      if (macIsEmpty(mac)) {
        strcpy(macStr, "--");
      } else {
        macToStr(mac, macStr);
      }
      lv_label_set_text(uic_Settings1LabelTSSPairMAC, macStr);
    }
  }
  // let JudgeModule lock or unlock the judge related parts
  JudgeModule::applyJudgeModeToSettingsScreen();
}

void updateMainScreen() {
  PullState pullState = StateManager::getPullState();
  const bool isMetric = (StateManager::getUnitSystem() == UnitSystem::METRIC);
  const bool tachVisible = StateManager::getTachEnabled();
  const bool limitVisible = StateManager::getLimitSwitchesEnabled();
  const bool relayVisible = StateManager::getRelaysEnabled();

  if (uic_MainContainerTach && tachVisible != lastDisplayedTachVisible) {
    set_visible(uic_MainContainerTach, tachVisible);
    lastDisplayedTachVisible = tachVisible;
  }
  if (uic_MainContainerLimit && limitVisible != lastDisplayedLimitVisible) {
    set_visible(uic_MainContainerLimit, limitVisible);
    lastDisplayedLimitVisible = limitVisible;
  }
  if (uic_MainContainerRelays && relayVisible != lastDisplayedRelayVisible) {
    set_visible(uic_MainContainerRelays, relayVisible);
    lastDisplayedRelayVisible = relayVisible;
  }

  bool showMax = (pullState == PullState::PULLEND);
  RemoteManager::SetIsMax(showMax);

  RemoteManager::SetTelemetry(StateManager::getSpeed(),
                            StateManager::getRPM(),
                            StateManager::getDistance());

  // Speed
  float speed = showMax ? StateManager::getMaxSpeed() : StateManager::getSpeed();
  if (uic_MainLabelSpeedUnit && lastDisplayedIsMetric != isMetric) {
    lv_label_set_text(uic_MainLabelSpeedUnit, isMetric ? "km/h" : "mph");
  }
  if (uic_MainLabelSpeedValue && speed != lastDisplayedSpeed) {
    if (speed < 0 || isnan(speed)) {
      lv_label_set_text(uic_MainLabelSpeedValue, "--.-");
    } else {
      char buf[16];
      snprintf(buf, sizeof(buf), "%.1f", speed);
      lv_label_set_text(uic_MainLabelSpeedValue, buf);
    }
    lastDisplayedSpeed = speed;
  }

  // Distance
  float distance = showMax ? StateManager::getMaxDistance() : StateManager::getDistance();
  if (uic_MainLabelDistanceUnit && lastDisplayedIsMetric != isMetric) {
    lv_label_set_text(uic_MainLabelDistanceUnit, isMetric ? "m" : "ft");
  }
  if (uic_MainLabelDistanceValue && distance != lastDisplayedDistance) {
    if (distance < 0 || isnan(distance)) {
      lv_label_set_text(uic_MainLabelDistanceValue, "---.--");
    } else {
      char buf[16];
      snprintf(buf, sizeof(buf), "%.2f", distance);
      lv_label_set_text(uic_MainLabelDistanceValue, buf);
    }
    lastDisplayedDistance = distance;
  }

  // RPM
  float rpm = showMax ? StateManager::getMaxRPM() : StateManager::getRPM();
  if (uic_MainLabelTachValue && rpm != lastDisplayedRPM) {
    if (rpm < 0 || isnan(rpm)) {
      lv_label_set_text(uic_MainLabelTachValue, "---");
    } else {
      char buf[16];
      snprintf(buf, sizeof(buf), "%.0f", rpm);
      lv_label_set_text(uic_MainLabelTachValue, buf);
    }
    lastDisplayedRPM = rpm;
  }

  if (ui_MainLabelDistanceTitle && lastDisplayedDistanceTitleVisible != showMax) {
    if (showMax) {
      lv_obj_clear_flag(ui_MainLabelDistanceTitle, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(ui_MainLabelDistanceTitle, LV_OBJ_FLAG_HIDDEN);
    }
    lastDisplayedDistanceTitleVisible = showMax;
  }

  if (ui_MainLabelSpeedTitle && lastDisplayedShowMax != showMax) {
    lv_label_set_text(ui_MainLabelSpeedTitle, showMax ? "MAX SPEED" : "Speed");
  }

  if (ui_MainLabelTachTitle && lastDisplayedShowMax != showMax) {
    lv_label_set_text(ui_MainLabelTachTitle, showMax ? "MAX RPM" : "Tach");
  }

  // Driver info
  const bool judgeMode = StateManager::getJudgeMode();
  std::string driverName = judgeMode ? JudgeModule::getDisplayDriverName()
                                     : StateManager::prefs().driverName.c_str();
  int driverNumber = judgeMode ? JudgeModule::getDisplayDriverNumber()
                               : StateManager::prefs().driverNumber;
  if (driverName != lastDisplayedDriverName || driverNumber != lastDisplayedDriverNumber) {
    lv_label_set_text_fmt(uic_MainLabelDriverName, "%s", driverName.c_str());
    lv_label_set_text_fmt(uic_MainLabelDriverNumber, "#%d", driverNumber);
    lastDisplayedDriverName = driverName;
    lastDisplayedDriverNumber = driverNumber;
  }

  // Class name
  std::string className = judgeMode ? (std::string("M4 Remote Monitor - ") + DEVICE_VERSION)
                                    : StateManager::prefs().pullingClassName.c_str();
  if (className != lastDisplayedClassName) {
    lv_label_set_text_fmt(uic_MainLabelClassName, "%s", className.c_str());
    lastDisplayedClassName = className;
  }

  // M4 Unit ID Number
  int unitId = judgeMode ? JudgeModule::getDisplayHostUnitId()
                         : StateManager::prefs().M4IDNumber;
  if (unitId != lastDisplayedUnitID) {
    lv_label_set_text_fmt(ui_MainLabelM4IDNumberLabel, "%d", unitId);
    lastDisplayedUnitID = unitId;

    // Update the Judge Setting Tips label with the correct ID
    lv_label_set_text_fmt(
      uic_Settings1LabelJudgeSettingTips,
        "1. On the Judge unit, enable Judge Stand Mode in General Settings.\n\n"
        "2. Open the Judge tab and enter MCU ID %d.\n\n"
        "3. Tap Connect. The Judge will begin receiving pull state and sensor data.",
      unitId);
  }

  uint8_t presetIdx = AlarmManager::getActivePreset();
  if (presetIdx != lastDisplayedPresetIdx) {
    if (uic_MainLabelAlarmPresetLable) {
      lv_label_set_text_fmt(uic_MainLabelAlarmPresetLable, "%u", static_cast<unsigned>(presetIdx) + 1U);
    }
    if (uic_MainContainerAlarmPresetContainer) {
      size_t colorIdx = (presetIdx < sizeof(kPresetColors) / sizeof(kPresetColors[0])) ? presetIdx : 0;
      lv_color_t color = kPresetColors[colorIdx];
      lv_obj_set_style_bg_color(uic_MainContainerAlarmPresetContainer, color, LV_PART_MAIN | LV_STATE_DEFAULT);
      lv_obj_set_style_bg_opa(uic_MainContainerAlarmPresetContainer, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lastDisplayedPresetIdx = presetIdx;
  }

  updateAlarmIndicators();

  const bool silenceAlarmVisible = AlarmManager::hasActiveAudibleAlarm();
  if (uic_MainButtonsilenceAlarmButton && silenceAlarmVisible != lastDisplayedSilenceAlarmVisible) {
    set_visible(uic_MainButtonsilenceAlarmButton, silenceAlarmVisible);
    lastDisplayedSilenceAlarmVisible = silenceAlarmVisible;
  }

  //
  // Limit Switch Indicators
  //
  for (int i = 0; i < 2; ++i) {
    bool enabled = limitVisible && StateManager::isLimitSwitchEnabled(i);
    bool triggered = StateManager::getLimitSwitchTriggered(i);

    setIndicatorColor(
        (i == 0 ? uic_MainPanelLimitIndicatorIcon1 : uic_MainPanelLimitIndicatorIcon2),
        enabled ? (triggered ? UI_THEME_COLOR_INDICGREEN : UI_THEME_COLOR_INDICRED) : UI_THEME_COLOR_INDICDISABLED);
  }

  //
  // Relay Indicators
  //
  for (int i = 0; i < 4; ++i) {
    lv_obj_t* icon = nullptr;
    switch (i) {
      case 0:
        icon = uic_MainPanelRelayIndicatorIcon1;
        break;
      case 1:
        icon = uic_MainPanelRelayIndicatorIcon2;
        break;
      case 2:
        icon = uic_MainPanelRelayIndicatorIcon3;
        break;
      case 3:
        icon = uic_MainPanelRelayIndicatorIcon4;
        break;
    }

    if (!relayVisible || !StateManager::isRelayEnabled(i)) {
      setIndicatorColor(icon, UI_THEME_COLOR_INDICDISABLED);
    } else {
      RelayState state = StateManager::state().relayStates[i];
      setIndicatorColor(icon, state == RelayState::ENGAGED ? UI_THEME_COLOR_INDICGREEN : UI_THEME_COLOR_INDICRED);
    }
  }

  // Distance Progress bar Graphs
  // Track bar graph updates
  float trackLength = StateManager::getTrackLength();
  float currentDistance = StateManager::getDistance();

  auto cfgD1 = AlarmManager::getConfigActive(AlarmChannel::DISTANCE, AlarmSlot::A1);
  auto cfgD2 = AlarmManager::getConfigActive(AlarmChannel::DISTANCE, AlarmSlot::A2);

  float alarm1 = AlarmManager::baseToUi(AlarmChannel::DISTANCE, cfgD1.tripPoint);
  float alarm2 = AlarmManager::baseToUi(AlarmChannel::DISTANCE, cfgD2.tripPoint);

  // Set min/max range (always 0 to track length)
  const int trackLengthInt = (int)trackLength;
  const int currentDistanceInt = (int)currentDistance;
  if (ui_MainBarDistanceProgress && lastDisplayedTrackLength != trackLengthInt) {
    lv_bar_set_range(ui_MainBarDistanceProgress, 0, trackLengthInt);
  }
  // lv_bar_set_range(ui_MainBarDistanceAlarm1, 0, (int)trackLength);
  // lv_bar_set_range(ui_MainBarDistanceAlarm2, 0, (int)trackLength);

  // Set values
  if (ui_MainBarDistanceProgress && lastDisplayedProgressDistance != currentDistanceInt) {
    lv_bar_set_value(ui_MainBarDistanceProgress, currentDistanceInt, LV_ANIM_OFF);
    lastDisplayedProgressDistance = currentDistanceInt;
  }
  // lv_bar_set_value(ui_MainBarDistanceAlarm1, (int)alarm1, LV_ANIM_OFF);
  // lv_bar_set_value(ui_MainBarDistanceAlarm2, (int)alarm2, LV_ANIM_OFF);

  if (!uic_MainLabelSpeedValue) LOGE("uic_MainLabelSpeedValue is NULL");
  if (!uic_MainLabelDistanceValue) LOGE("uic_MainLabelDistanceValue is NULL");
  if (!uic_MainLabelTachValue) LOGE("uic_MainLabelTachValue is NULL");

    // TSS connection icons
  {
    bool tssConnected = Tach::state.isTSSConnected;
    if (lastDisplayedTssConnected != tssConnected) {
      // when NOT connected: show 'tachConnNo', hide 'tachConn'
      set_visible(uic_MainImagetachConnNo, !tssConnected);
      set_visible(uic_MainImagetachConn,    tssConnected);
      lastDisplayedTssConnected = tssConnected;
    }
  }

  // Apply judge locks for the main screen buttons and alarm preset container
  JudgeModule::applyJudgeModeToMainScreen();
  lastDisplayedTrackLength = trackLengthInt;
  lastDisplayedIsMetric = isMetric;
  lastDisplayedShowMax = showMax;
}

static inline void set_bar_visible(lv_obj_t* bar, bool visible) {
  if (!bar) return;
  if (visible) lv_obj_clear_flag(bar, LV_OBJ_FLAG_HIDDEN);
  else         lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN);
}

static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

extern lv_obj_t* uic_Settings1ButtonPresetButton1;
extern lv_obj_t* uic_Settings1ButtonPresetButton2;
extern lv_obj_t* uic_Settings1ButtonPresetButton3;
extern lv_obj_t* uic_Settings1ButtonPresetButton4;

static void setPresetButtons(uint8_t preset) {
  lv_obj_t* btns[4] = {
    uic_Settings1ButtonPresetButton1,
    uic_Settings1ButtonPresetButton2,
    uic_Settings1ButtonPresetButton3,
    uic_Settings1ButtonPresetButton4
  };
  for (int i = 0; i < 4; ++i) {
    if (!btns[i]) continue;
    // clear all first
    lv_obj_clear_state(btns[i], LV_STATE_CHECKED);
  }
  if (preset >= 4) preset = 0;
  if (btns[preset]) lv_obj_add_state(btns[preset], LV_STATE_CHECKED);
}

void refreshAlarmUIFromPreset(uint8_t preset) {
  alarmUiRefreshing = true;

  setPresetButtons(preset); 

  struct AlarmUiBinding {
    AlarmChannel ch;
    AlarmSlot sl;
    lv_obj_t* toggle;
    lv_obj_t* value;
    lv_obj_t* style;
    lv_obj_t* color;
  };

  const AlarmUiBinding bindings[] = {
      {AlarmChannel::DISTANCE, AlarmSlot::A1, ui_Settings1SwitchAlarmDistanceToggle1,
       ui_Settings1TextareaAlarmDistanceValueTextArea1, ui_Settings1DropdownAlarmDistanceDropdown1,
       ui_Settings1DropdownAlarmDistanceColorDropdown1},
      {AlarmChannel::DISTANCE, AlarmSlot::A2, ui_Settings1SwitchAlarmDistanceToggle2,
       ui_Settings1TextareaAlarmDistanceValueTextArea2, ui_Settings1DropdownAlarmDistanceDropdown2,
       ui_Settings1DropdownAlarmDistanceColorDropdown2},
      {AlarmChannel::SPEED, AlarmSlot::A1, ui_Settings1SwitchAlarmSpeedToggle1,
       ui_Settings1TextareaAlarmSpeedValueTextArea1, ui_Settings1DropdownAlarmSpeedDropdown1,
       ui_Settings1DropdownAlarmSpeedColorDropdown1},
      {AlarmChannel::SPEED, AlarmSlot::A2, ui_Settings1SwitchAlarmSpeedToggle2,
       ui_Settings1TextareaAlarmSpeedValueTextArea2, ui_Settings1DropdownAlarmSpeedDropdown2,
       ui_Settings1DropdownAlarmSpeedColorDropdown2},
      {AlarmChannel::RPM, AlarmSlot::A1, ui_Settings1SwitchAlarmRPMToggle1, ui_Settings1TextareaAlarmRPMValueTextArea1,
       ui_Settings1DropdownAlarmRPMDropdown1, ui_Settings1DropdownAlarmRPMColorDropdown1},
      {AlarmChannel::RPM, AlarmSlot::A2, ui_Settings1SwitchAlarmRPMToggle2, ui_Settings1TextareaAlarmRPMValueTextArea2,
       ui_Settings1DropdownAlarmRPMDropdown2, ui_Settings1DropdownAlarmRPMColorDropdown2}};

  for (const auto& binding : bindings) {
    AlarmConfig cfg = AlarmManager::getConfig(preset, binding.ch, binding.sl);
    float uiValue = AlarmManager::baseToUi(binding.ch, cfg.tripPoint);

    if (binding.toggle) {
      if (cfg.enabled) {
        if (!lv_obj_has_state(binding.toggle, LV_STATE_CHECKED)) {
          lv_obj_add_state(binding.toggle, LV_STATE_CHECKED);
        }
      } else {
        lv_obj_clear_state(binding.toggle, LV_STATE_CHECKED);
      }
    }

    if (binding.value) {
      if (cfg.enabled) {
        lv_obj_clear_state(binding.value, LV_STATE_DISABLED);
      } else {
        lv_obj_add_state(binding.value, LV_STATE_DISABLED);
      }
    }
    if (binding.style) {
      if (cfg.enabled) {
        lv_obj_clear_state(binding.style, LV_STATE_DISABLED);
      } else {
        lv_obj_add_state(binding.style, LV_STATE_DISABLED);
      }
    }
    if (binding.color) {
      if (cfg.enabled) {
        lv_obj_clear_state(binding.color, LV_STATE_DISABLED);
      } else {
        lv_obj_add_state(binding.color, LV_STATE_DISABLED);
      }
    }

    if (binding.value) {
      char buf[16];
      switch (binding.ch) {
        case AlarmChannel::DISTANCE:
          snprintf(buf, sizeof(buf), "%.2f", uiValue);
          break;
        case AlarmChannel::SPEED:
          snprintf(buf, sizeof(buf), "%.1f", uiValue);
          break;
        case AlarmChannel::RPM:
        default:
          snprintf(buf, sizeof(buf), "%.0f", uiValue);
          break;
      }
      
      // For RPM (no decimals), don't trim anything
      // For distance/speed, only trim trailing zeros AFTER decimal point
      if (binding.ch != AlarmChannel::RPM) {
        // Only trim if there's a decimal point
        char* dot = strchr(buf, '.');
        if (dot) {
          for (int i = strlen(buf) - 1; i > 0 && buf[i] == '0'; --i) {
            buf[i] = '\0';
            if (buf[i - 1] == '.') {
              buf[i - 1] = '\0';
              break;
            }
          }
        }
      }
      
      lv_textarea_set_text(binding.value, buf);
    }

    if (binding.style) {
      lv_dropdown_set_selected(binding.style, AlarmManager::mapStyleToIndex(cfg.style));
    }

    if (binding.color) {
      lv_dropdown_set_selected(binding.color, AlarmManager::mapColorToIndex(cfg.color));
    }
  }

  float trackLength = StateManager::getTrackLength();
  auto cfgD1 = AlarmManager::getConfig(preset, AlarmChannel::DISTANCE, AlarmSlot::A1);
  auto cfgD2 = AlarmManager::getConfig(preset, AlarmChannel::DISTANCE, AlarmSlot::A2);
  float alarm1 = AlarmManager::baseToUi(AlarmChannel::DISTANCE, cfgD1.tripPoint);
  float alarm2 = AlarmManager::baseToUi(AlarmChannel::DISTANCE, cfgD2.tripPoint);

  bool en1 = cfgD1.enabled;
  bool en2 = cfgD2.enabled;

  int maxv = (int)trackLength;

  if (ui_MainBarDistanceAlarm1) {
    LOGD("Setting alarm1 bar visible=%d", en1);
    set_bar_visible(ui_MainBarDistanceAlarm1, en1);
    if (en1) {
      applyDistanceAlarmColor(ui_MainBarDistanceAlarm1, cfgD1.color);
      lv_bar_set_range(ui_MainBarDistanceAlarm1, 0, maxv);
      lv_bar_set_value(ui_MainBarDistanceAlarm1, clampi((int)alarm1, 0, maxv), LV_ANIM_OFF);
    }
  }

  if (ui_MainBarDistanceAlarm2) {
    set_bar_visible(ui_MainBarDistanceAlarm2, en2);
    if (en2) {
      applyDistanceAlarmColor(ui_MainBarDistanceAlarm2, cfgD2.color);
      lv_bar_set_range(ui_MainBarDistanceAlarm2, 0, maxv);
      lv_bar_set_value(ui_MainBarDistanceAlarm2, clampi((int)alarm2, 0, maxv), LV_ANIM_OFF);
    }
  }

  // Update triangle labels for distance alarms
  if (ui_MainLabelDistanceAlarmTriangle1) {
    LOGD("Updating Alarm1 triangle, enabled=%d", en1);
    if (en1) {
      lv_obj_move_foreground(ui_MainLabelDistanceAlarmTriangle1);
      lv_obj_clear_flag(ui_MainLabelDistanceAlarmTriangle1, LV_OBJ_FLAG_HIDDEN);
      applyDistanceAlarmColorToLabel(ui_MainLabelDistanceAlarmTriangle1, cfgD1.color);
      float percentage = (alarm1 / trackLength) * 100.0f;
      int yPos = 240 - (int)(percentage * 4.67f);
      LOGD("Alarm1 percentage: %.2f%%, yPos: %d", percentage, yPos);
      lv_obj_set_pos(ui_MainLabelDistanceAlarmTriangle1, -354, yPos);
    } else {
      lv_obj_add_flag(ui_MainLabelDistanceAlarmTriangle1, LV_OBJ_FLAG_HIDDEN);
    }
  }

  if (ui_MainLabelDistanceAlarmTriangle2) {
    LOGD("Updating Alarm2 triangle, enabled=%d", en2);
    if (en2) {
      lv_obj_move_foreground(ui_MainLabelDistanceAlarmTriangle2);
      lv_obj_clear_flag(ui_MainLabelDistanceAlarmTriangle2, LV_OBJ_FLAG_HIDDEN);
      applyDistanceAlarmColorToLabel(ui_MainLabelDistanceAlarmTriangle2, cfgD2.color);
      float percentage = (alarm2 / trackLength) * 100.0f;
      int yPos = 239 - (int)(percentage * 4.67f);
      LOGD("Alarm2 percentage: %.2f%%, yPos: %d", percentage, yPos);
      lv_obj_set_pos(ui_MainLabelDistanceAlarmTriangle2, -354, yPos);
    } else {
      lv_obj_add_flag(ui_MainLabelDistanceAlarmTriangle2, LV_OBJ_FLAG_HIDDEN);
    }
  }

  alarmUiRefreshing = false;
}

void updateAlarmIndicators() {
  // Helper to get indicator color INDEX based on alarm config
  auto getAlarmColorIndex = [](AlarmColor col) -> uint8_t {
    switch (col) {
      case AlarmColor::RED:    return UI_THEME_COLOR_INDICRED;
      case AlarmColor::YELLOW: return UI_THEME_COLOR_INDICYELLOW;
      case AlarmColor::GREEN:  return UI_THEME_COLOR_INDICGREEN;
      default:                 return UI_THEME_COLOR_INDICDISABLED;
    }
  };
  
  // Get active alarm configs
  auto speedA1 = AlarmManager::getConfigActive(AlarmChannel::SPEED, AlarmSlot::A1);
  auto speedA2 = AlarmManager::getConfigActive(AlarmChannel::SPEED, AlarmSlot::A2);
  auto rpmA1 = AlarmManager::getConfigActive(AlarmChannel::RPM, AlarmSlot::A1);
  auto rpmA2 = AlarmManager::getConfigActive(AlarmChannel::RPM, AlarmSlot::A2);
  auto distA1 = AlarmManager::getConfigActive(AlarmChannel::DISTANCE, AlarmSlot::A1);
  auto distA2 = AlarmManager::getConfigActive(AlarmChannel::DISTANCE, AlarmSlot::A2);

  int fullOpacity = 255;
  int faintOpacity = 30;
  
  // RPM/Tach Alarm 1
  if (rpmA1.enabled) {
    set_visible(uic_MainPanelTachAlarmIndicatorIcon1, true);
    uint8_t colorIdx = getAlarmColorIndex(rpmA1.color);
    uint8_t opacity = rpmA1.tripped ? fullOpacity : faintOpacity;  // Bright when tripped, very faint when not
    setIndicatorColor(uic_MainPanelTachAlarmIndicatorIcon1, colorIdx, opacity);
  } else {
    set_visible(uic_MainPanelTachAlarmIndicatorIcon1, false);  // HIDDEN when disabled
  }
  
  // RPM/Tach Alarm 2
  if (rpmA2.enabled) {
    set_visible(uic_MainPanelTachAlarmIndicatorIcon2, true);
    uint8_t colorIdx = getAlarmColorIndex(rpmA2.color);
    uint8_t opacity = rpmA2.tripped ? fullOpacity : faintOpacity;
    setIndicatorColor(uic_MainPanelTachAlarmIndicatorIcon2, colorIdx, opacity);
  } else {
    set_visible(uic_MainPanelTachAlarmIndicatorIcon2, false);  // HIDDEN when disabled
  }
  
  // Speed Alarm 1
  if (speedA1.enabled) {
    set_visible(uic_MainPanelSpeedAlarmIndicatorIcon1, true);
    uint8_t colorIdx = getAlarmColorIndex(speedA1.color);
    uint8_t opacity = speedA1.tripped ? fullOpacity : faintOpacity;
    setIndicatorColor(uic_MainPanelSpeedAlarmIndicatorIcon1, colorIdx, opacity);
  } else {
    set_visible(uic_MainPanelSpeedAlarmIndicatorIcon1, false);  // HIDDEN when disabled
  }
  
  // Speed Alarm 2
  if (speedA2.enabled) {
    set_visible(uic_MainPanelSpeedAlarmIndicatorIcon2, true);
    uint8_t colorIdx = getAlarmColorIndex(speedA2.color);
    uint8_t opacity = speedA2.tripped ? fullOpacity : faintOpacity;
    setIndicatorColor(uic_MainPanelSpeedAlarmIndicatorIcon2, colorIdx, opacity);
  } else {
    set_visible(uic_MainPanelSpeedAlarmIndicatorIcon2, false);  // HIDDEN when disabled
  }

  setDistanceAlarmTripState(ui_MainBarDistanceAlarm1, ui_MainLabelDistanceAlarmTriangle1,
                            distA1.color, distA1.enabled, distA1.tripped);
  setDistanceAlarmTripState(ui_MainBarDistanceAlarm2, ui_MainLabelDistanceAlarmTriangle2,
                            distA2.color, distA2.enabled, distA2.tripped);
}
