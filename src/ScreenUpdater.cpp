#include "ScreenUpdater.h"

#include "../ui/ui.h"
#include "PullStateManager.h"
#include "StateManager.h"

// Last known values to avoid unnecessary redraws
static float lastDisplayedSpeed = -1.0f;
static float lastDisplayedDistance = -1.0f;
static float lastDisplayedRPM = -1.0f;
static std::string lastClassName;
static std::string lastDriverName;
static int lastDriverNumber = -1;

static void setIndicatorColor(lv_obj_t* obj, uint8_t themeID) {
  lv_color_t color = COLOR_INDIC_DISABLED;

  switch (themeID) {
    case UI_THEME_COLOR_INDICGREEN:
      color = COLOR_INDIC_GREEN;
      break;
    case UI_THEME_COLOR_INDICRED:
      color = COLOR_INDIC_RED;
      break;
    case UI_THEME_COLOR_INDICDISABLED:
      color = COLOR_INDIC_DISABLED;
      break;
    case UI_THEME_COLOR_YELLOW:
      color = COLOR_INDIC_YELLOW;
      break;
  }

  lv_obj_set_style_bg_color(obj, color, 0);
}

void updateMainScreen() {
  PullState pullState = StateManager::getPullState();
  bool showMax = (pullState == PullState::PULLEND);

  // Speed
  float speed = showMax ? StateManager::getMaxSpeed() : StateManager::getSpeed();
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

  if (ui_MainLabelDistanceTitle) {
    if (showMax) {
      lv_obj_clear_flag(ui_MainLabelDistanceTitle, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(ui_MainLabelDistanceTitle, LV_OBJ_FLAG_HIDDEN);
    }
  }

  if (ui_MainLabelSpeedTitle) {
    lv_label_set_text(ui_MainLabelSpeedTitle, showMax ? "MAX SPEED" : "Speed");
  }

  if (ui_MainLabelTachTitle) {
    lv_label_set_text(ui_MainLabelTachTitle, showMax ? "MAX RPM" : "Tach");
  }

  // Driver info
  std::string driverName = StateManager::prefs().driverName.c_str();
  int driverNumber = StateManager::prefs().driverNumber;
  if (driverName != lastDriverName || driverNumber != lastDriverNumber) {
    lv_label_set_text_fmt(uic_MainLabelDriverName, "%s", driverName.c_str());
    lv_label_set_text_fmt(uic_MainLabelDriverNumber, "#%d", driverNumber);
    lastDriverName = driverName;
    lastDriverNumber = driverNumber;
  }

  // Class name
  std::string className = StateManager::prefs().pullingClassName.c_str();
  if (className != lastClassName) {
    lv_label_set_text_fmt(uic_MainLabelClassName, "%s", className.c_str());
    lastClassName = className;
  }

  //
  // Tach Alarm Indicator
  //
  float tach1 = StateManager::prefs().tachAlarm1;
  float tach2 = StateManager::prefs().tachAlarm2;

  if (rpm > tach1 && rpm > tach2) {
    setIndicatorColor(uic_MainPanelTachAlarmIndicatorIcon, UI_THEME_COLOR_INDICRED);
  } else if (rpm > tach1 || rpm > tach2) {
    setIndicatorColor(uic_MainPanelTachAlarmIndicatorIcon, UI_THEME_COLOR_YELLOW);
  } else {
    setIndicatorColor(uic_MainPanelTachAlarmIndicatorIcon, UI_THEME_COLOR_INDICDISABLED);
  }

  //
  // Speed Alarm Indicator
  //
  float speed1 = StateManager::prefs().mphAlarm1;
  float speed2 = StateManager::prefs().mphAlarm2;

  if (speed > speed1 && speed > speed2) {
    setIndicatorColor(uic_MainPanelSpeedAlarmIndicatorIcon, UI_THEME_COLOR_INDICRED);
  } else if (speed > speed1 || speed > speed2) {
    setIndicatorColor(uic_MainPanelSpeedAlarmIndicatorIcon, UI_THEME_COLOR_YELLOW);
  } else {
    setIndicatorColor(uic_MainPanelSpeedAlarmIndicatorIcon, UI_THEME_COLOR_INDICDISABLED);
  }

  //
  // Limit Switch Indicators
  //
  for (int i = 0; i < 2; ++i) {
    bool enabled = StateManager::isLimitSwitchEnabled(i);
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

    if (!StateManager::prefs().relayEnabled[i]) {
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
  float alarm1 = StateManager::prefs().distanceAlarm1;
  float alarm2 = StateManager::prefs().distanceAlarm2;

  // Set min/max range (always 0 to track length)
  lv_bar_set_range(ui_MainBarDistanceProgress, 0, (int)trackLength);
  lv_bar_set_range(ui_MainBarDistanceAlarm1, 0, (int)trackLength);
  lv_bar_set_range(ui_MainBarDistanceAlarm2, 0, (int)trackLength);

  // Set values
  lv_bar_set_value(ui_MainBarDistanceProgress, (int)currentDistance, LV_ANIM_OFF);
  lv_bar_set_value(ui_MainBarDistanceAlarm1, (int)alarm1, LV_ANIM_OFF);
  lv_bar_set_value(ui_MainBarDistanceAlarm2, (int)alarm2, LV_ANIM_OFF);

  if (!uic_MainLabelSpeedValue) Serial.println("❌ uic_MainLabelSpeedValue is NULL");
  if (!uic_MainLabelDistanceValue) Serial.println("❌ uic_MainLabelDistanceValue is NULL");
  if (!uic_MainLabelTachValue) Serial.println("❌ uic_MainLabelTachValue is NULL");
}
