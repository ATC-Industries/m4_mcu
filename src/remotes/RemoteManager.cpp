#include "remotes/RemoteManager.h"

#include <cstring>
#include "Arduino.h"
#include "ArduinoJson.h"
#include "M4MessageStruct.h"
#include "StateManager.h"
#include "Logging.h"
#include "ui/ui.h"
#include "lvgl.h"
#include "M4CommsHelpers.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

// -----------------------------------------------------------------------------
// Small critical section to guard rows_ from ISR vs loop
// -----------------------------------------------------------------------------
static portMUX_TYPE s_rows_mux = portMUX_INITIALIZER_UNLOCKED;
#define LOCK_ROWS()   portENTER_CRITICAL(&s_rows_mux)
#define UNLOCK_ROWS() portEXIT_CRITICAL(&s_rows_mux)

// Your existing sender
extern esp_err_t sendMessage(const uint8_t *peerAddress,
                             msg_type messageType,
                             action_type messageAction,
                             String payload,
                             priority messagePriority);

// -----------------------------------------------------------------------------
// Cadences and limits
// -----------------------------------------------------------------------------
static constexpr uint32_t kDiscoverEveryMs = 500;   // pairing discover
static constexpr uint32_t kRowStaleMs      = 5000;  // remove dead entries
static constexpr uint32_t kPurgeEveryMs    = 1000;  // clean list
static constexpr uint32_t kValuesEveryMs   = 200;   // 5 Hz runtime broadcast
static constexpr size_t   kMaxRowsShown    = 6;

// -----------------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------------
static std::vector<RemoteRow> s_rows;
static bool s_pairing_on = false;

// timers
static uint32_t s_last_discover_ms = 0;
static uint32_t s_last_purge_ms    = 0;
static uint32_t s_last_values_ms   = 0;

// telemetry cache
static float s_cur_speed  = 0.0f;
static float s_cur_rpm    = 0.0f;
static float s_cur_dist   = 0.0f;
static bool  s_cur_ismax  = false;
static bool  s_cur_safety = false;

// UI wiring
static lv_obj_t* s_table_container   = nullptr;
static lv_obj_t* s_status_label      = nullptr;  // uic_Settings1LabelRemoteSearching
static lv_obj_t* s_status_spinner    = nullptr;  // uic_Settings1SpinnerRemoteSearchingSpinner

struct RowUi {
  lv_obj_t* container;
  lv_obj_t* type_label;
  lv_obj_t* mac_label;
  lv_obj_t* host_label;
  lv_obj_t* action_button;
  lv_obj_t* action_label;
};

// row 0..5 map to rows 1..6 in the UI
static RowUi s_row_ui[kMaxRowsShown] = {};

static volatile bool s_needs_ui_refresh = false;

// -----------------------------------------------------------------------------
// Helpers to set up row UI pointers when screen is active
// -----------------------------------------------------------------------------
static void InitRowUiPointers() {
  // Row 1
  s_row_ui[0].container      = uic_Settings1ContainerDeviceTableRow1;
  s_row_ui[0].type_label     = uic_Settings1LabelDeviceRowTypeLabel1;
  s_row_ui[0].mac_label      = uic_Settings1LabelDeviceRowMACLabel1;
  s_row_ui[0].host_label     = uic_Settings1LabelDeviceRowHostLabel1;
  s_row_ui[0].action_button  = uic_Settings1ButtonDeviceRowActionButton1;
  s_row_ui[0].action_label   = uic_Settings1LabelDeviceRowActionButtonLabel1;

  // Row 2
  s_row_ui[1].container      = uic_Settings1ContainerDeviceTableRow2;
  s_row_ui[1].type_label     = uic_Settings1LabelDeviceRowTypeLabel2;
  s_row_ui[1].mac_label      = uic_Settings1LabelDeviceRowMACLabel2;
  s_row_ui[1].host_label     = uic_Settings1LabelDeviceRowHostLabel2;
  s_row_ui[1].action_button  = uic_Settings1ButtonDeviceRowActionButton2;
  s_row_ui[1].action_label   = uic_Settings1LabelDeviceRowActionButtonLabel2;

  // Row 3
  s_row_ui[2].container      = uic_Settings1ContainerDeviceTableRow3;
  s_row_ui[2].type_label     = uic_Settings1LabelDeviceRowTypeLabel3;
  s_row_ui[2].mac_label      = uic_Settings1LabelDeviceRowMACLabel3;
  s_row_ui[2].host_label     = uic_Settings1LabelDeviceRowHostLabel3;
  s_row_ui[2].action_button  = uic_Settings1ButtonDeviceRowActionButton3;
  s_row_ui[2].action_label   = uic_Settings1LabelDeviceRowActionButtonLabel3;

  // Row 4
  s_row_ui[3].container      = uic_Settings1ContainerDeviceTableRow4;
  s_row_ui[3].type_label     = uic_Settings1LabelDeviceRowTypeLabel4;
  s_row_ui[3].mac_label      = uic_Settings1LabelDeviceRowMACLabel4;
  s_row_ui[3].host_label     = uic_Settings1LabelDeviceRowHostLabel4;
  s_row_ui[3].action_button  = uic_Settings1ButtonDeviceRowActionButton4;
  s_row_ui[3].action_label   = uic_Settings1LabelDeviceRowActionButtonLabel4;

  // Row 5
  s_row_ui[4].container      = uic_Settings1ContainerDeviceTableRow5;
  s_row_ui[4].type_label     = uic_Settings1LabelDeviceRowTypeLabel5;
  s_row_ui[4].mac_label      = uic_Settings1LabelDeviceRowMACLabel5;
  s_row_ui[4].host_label     = uic_Settings1LabelDeviceRowHostLabel5;
  s_row_ui[4].action_button  = uic_Settings1ButtonDeviceRowActionButton5;
  s_row_ui[4].action_label   = uic_Settings1LabelDeviceRowActionButtonLabel5;

  // Row 6
  s_row_ui[5].container      = uic_Settings1ContainerDeviceTableRow6;
  s_row_ui[5].type_label     = uic_Settings1LabelDeviceRowTypeLabel6;
  s_row_ui[5].mac_label      = uic_Settings1LabelDeviceRowMACLabel6;
  s_row_ui[5].host_label     = uic_Settings1LabelDeviceRowHostLabel6;
  s_row_ui[5].action_button  = uic_Settings1ButtonDeviceRowActionButton6;
  s_row_ui[5].action_label   = uic_Settings1LabelDeviceRowActionButtonLabel6;

  s_status_label   = uic_Settings1LabelRemoteSearching;
  s_status_spinner = uic_Settings1SpinnerRemoteSearchingSpinner;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void RemoteManager::Init() {
  LOCK_ROWS();
  s_rows.clear();
  UNLOCK_ROWS();

  s_pairing_on        = false;
  s_last_discover_ms  = 0;
  s_last_purge_ms     = 0;
  s_last_values_ms    = 0;
  s_cur_speed         = 0.0f;
  s_cur_rpm           = 0.0f;
  s_cur_dist          = 0.0f;
  s_cur_ismax         = false;
  s_cur_safety        = false;

  s_table_container   = nullptr;
  s_status_label      = nullptr;
  s_status_spinner    = nullptr;
  s_needs_ui_refresh  = false;

  for (size_t i = 0; i < kMaxRowsShown; ++i) {
    s_row_ui[i] = {};
  }
}

void RemoteManager::EnterPairing() {
  s_pairing_on       = true;
  s_last_discover_ms = 0;
  s_last_purge_ms    = 0;

  ClearAllRemotes();
  s_needs_ui_refresh = true;

  LOGI("[RemoteManager] Pairing ON");
  UpdateStatusLabel();
}

//TODO: Bug, not clearing remote lists items after ending Pair.
void RemoteManager::ExitPairing() {
  s_pairing_on = false;
  ClearAllRemotes();
  s_needs_ui_refresh = true;
  LOGI("[RemoteManager] Pairing OFF");
  UpdateStatusLabel();
}

bool RemoteManager::IsPairing() {
  return s_pairing_on;
}

void RemoteManager::Tick() {
  const uint32_t now = millis();

  if (s_pairing_on) {
    if (now - s_last_discover_ms >= kDiscoverEveryMs) {
      s_last_discover_ms = now;
      SendDiscover();
    }

    if (now - s_last_purge_ms >= kPurgeEveryMs) {
      s_last_purge_ms = now;

      bool changed = false;
      LOCK_ROWS();
      for (size_t i = 0; i < s_rows.size();) {
        if (now - s_rows[i].last_seen_ms > kRowStaleMs) {
          s_rows.erase(s_rows.begin() + i);
          changed = true;
        } else {
          ++i;
        }
      }
      UNLOCK_ROWS();

      if (changed) {
        s_needs_ui_refresh = true;
      }
    }
  }

  if (s_pairing_on && s_needs_ui_refresh) {
    s_needs_ui_refresh = false;
    RefreshListUI();
  }

  MaybeSendValues();
}

void RemoteManager::OnPairingRemoteResponse(const uint8_t* sender_mac,
                                            int rssi,
                                            const JsonDocument& doc) {
  if (!s_pairing_on) {
    return;
  }

  LOCK_ROWS();
  RemoteRow* rr = nullptr;
  if (!FindOrAddByMac(sender_mac, &rr) || rr == nullptr) {
    UNLOCK_ROWS();
    return;
  }

  rr->last_rssi    = rssi;
  rr->last_seen_ms = millis();

  if (doc["pairedHost"].is<int>()) {
    rr->paired_host = doc["pairedHost"].as<int>();
  }
  if (doc["type"].is<int>()) {
    rr->type = static_cast<remote_role>(doc["type"].as<int>());
  }
  if (doc["digits"].is<int>()) {
    rr->digits = static_cast<uint8_t>(doc["digits"].as<int>());
  }

  rr->is_this_host = (rr->paired_host == StateManager::getM4ID());

  UNLOCK_ROWS();

  s_needs_ui_refresh = true;
}

bool RemoteManager::ConnectRemote(const char* mac_str) {
  uint8_t mac[6];
  if (!StringToMac(mac_str, mac)) {
    return false;
  }
  bool ok = SendConnect(mac);
  if (!ok) {
    return false;
  }

  // optimistic UI update
  LOCK_ROWS();
  for (auto& r : s_rows) {
    if (strncmp(r.mac_str, mac_str, sizeof(r.mac_str)) == 0) {
      r.paired_host = StateManager::getM4ID();
      r.is_this_host = true;
      break;
    }
  }
  UNLOCK_ROWS();

  s_needs_ui_refresh = true;
  return true;
}

bool RemoteManager::DisconnectRemote(const char* mac_str) {
  uint8_t mac[6];
  if (!StringToMac(mac_str, mac)) {
    return false;
  }
  bool ok = SendDisconnect(mac);
  if (!ok) {
    return false;
  }

  LOCK_ROWS();
  for (auto& r : s_rows) {
    if (strncmp(r.mac_str, mac_str, sizeof(r.mac_str)) == 0) {
      r.paired_host  = 0;
      r.is_this_host = false;
      break;
    }
  }
  UNLOCK_ROWS();

  s_needs_ui_refresh = true;
  return true;
}

void RemoteManager::SetTelemetry(float speed, float rpm, float distance) {
  s_cur_speed = speed;
  s_cur_rpm   = rpm;
  s_cur_dist  = distance;
}

void RemoteManager::SetIsMax(bool isMaxNow) {
  s_cur_ismax = isMaxNow;
}

void RemoteManager::SetSafety(bool isSafetyNow) {
  s_cur_safety = isSafetyNow;
}

void RemoteManager::SetTableContainer(lv_obj_t* parent) {
  s_table_container = parent;

  if (s_table_container == nullptr) {
    // Screen going away
    ClearAllRowsUI();
    UpdateStatusLabel();
    return;
  }

  // Screen is now active
  InitRowUiPointers();

  // Hide all rows on entry
  for (size_t i = 0; i < kMaxRowsShown; ++i) {
    if (s_row_ui[i].container) {
      lv_obj_add_flag(s_row_ui[i].container, LV_OBJ_FLAG_HIDDEN);
    }
  }

  UpdateStatusLabel();
  s_needs_ui_refresh = true;
}

void RemoteManager::HandleRowButtonClicked(int row_index) {
  if (row_index < 0 || row_index >= static_cast<int>(kMaxRowsShown)) {
    return;
  }

  char mac_str[18] = {0};
  bool is_this_host = false;

  LOCK_ROWS();
  if (!s_rows.empty()
      && static_cast<size_t>(row_index) < s_rows.size()) {
    const RemoteRow& r = s_rows[static_cast<size_t>(row_index)];
    strncpy(mac_str, r.mac_str, sizeof(mac_str) - 1);
    mac_str[sizeof(mac_str) - 1] = '\0';
    is_this_host = r.is_this_host;
  }
  UNLOCK_ROWS();

  if (mac_str[0] == '\0') {
    return;
  }

  if (is_this_host) {
    DisconnectRemote(mac_str);
  } else {
    ConnectRemote(mac_str);
  }
}

// -----------------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------------
void RemoteManager::RefreshListUI() {
  if (!s_table_container) {
    return;
  }

  if (!s_pairing_on) {
    // keep rows hidden when not in pairing
    ClearAllRowsUI();
    return;
  }

  // Take a snapshot so we do not hold the lock while touching LVGL
  std::vector<RemoteRow> snapshot;
  {
    LOCK_ROWS();
    snapshot = s_rows;
    UNLOCK_ROWS();
  }

  const size_t count = snapshot.size();

  for (size_t i = 0; i < kMaxRowsShown; ++i) {
    RowUi& row_ui = s_row_ui[i];

    if (!row_ui.container
        || !row_ui.type_label
        || !row_ui.mac_label
        || !row_ui.host_label
        || !row_ui.action_button
        || !row_ui.action_label) {
      continue;
    }

    if (i >= count) {
      // no remote for this row, hide it
      lv_obj_add_flag(row_ui.container, LV_OBJ_FLAG_HIDDEN);
      continue;
    }

    const RemoteRow& r = snapshot[i];

    // show row
    lv_obj_clear_flag(row_ui.container, LV_OBJ_FLAG_HIDDEN);

    // type label
    const char* t =
      (r.type == REMOTE_SPEED)        ? "Speed" :
      (r.type == REMOTE_RPM)          ? "RPM" :
      (r.type == REMOTE_DISTANCE)     ? "Distance" :
      (r.type == REMOTE_SAFETY_LIGHT) ? "Safety" :
                                        "?";
    lv_label_set_text(row_ui.type_label, t);

    // MAC label
    lv_label_set_text(row_ui.mac_label, r.mac_str);

    // host label
    if (r.paired_host > 0) {
      if (r.is_this_host) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Host: %d (this)", r.paired_host);
        lv_label_set_text(row_ui.host_label, buf);
        lv_obj_set_style_text_color(
            row_ui.host_label, lv_color_hex(0x1FA709), 0);  // green
      } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "Host: %d", r.paired_host);
        lv_label_set_text(row_ui.host_label, buf);
        lv_obj_set_style_text_color(
            row_ui.host_label, lv_color_hex(0xA70909), 0);  // red
      }
    } else {
      lv_label_set_text(row_ui.host_label, "Host: -");
      lv_obj_set_style_text_color(
          row_ui.host_label, lv_color_hex(0x808080), 0);  // gray
    }

    // button label and color
    bool connected_to_us = r.is_this_host;
    if (connected_to_us) {
      lv_label_set_text(row_ui.action_label, "Disconnect");
      lv_obj_set_style_bg_color(
          row_ui.action_button, lv_color_hex(0xA70909),
          LV_PART_MAIN | LV_STATE_DEFAULT);  // red
      lv_obj_set_style_text_color(
          row_ui.action_label, lv_color_hex(0xFFFFFF),
          LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      lv_label_set_text(row_ui.action_label, "Connect");
      lv_obj_set_style_bg_color(
          row_ui.action_button, lv_color_hex(0x1FA709),
          LV_PART_MAIN | LV_STATE_DEFAULT);  // green
      lv_obj_set_style_text_color(
          row_ui.action_label, lv_color_hex(0xFFFFFF),
          LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  }
}

void RemoteManager::ClearAllRowsUI() {
  for (size_t i = 0; i < kMaxRowsShown; ++i) {
    if (s_row_ui[i].container) {
      lv_obj_add_flag(s_row_ui[i].container, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void RemoteManager::ClearAllRemotes() {
  LOCK_ROWS();
  s_rows.clear();
  UNLOCK_ROWS();
}

bool RemoteManager::FindOrAddByMac(const uint8_t mac[6], RemoteRow** out) {
  char s[18];
  MacToString(mac, s);

  for (;;) {
    LOCK_ROWS();
    // try to find existing
    for (auto& r : s_rows) {
      if (strncmp(r.mac_str, s, sizeof(r.mac_str)) == 0) {
        if (out != nullptr) {
          *out = &r;
        }
        UNLOCK_ROWS();
        return true;
      }
    }

    // cap at 6 devices in the list
    if (s_rows.size() >= kMaxRowsShown) {
      UNLOCK_ROWS();
      return false;
    }

    RemoteRow r{};
    memcpy(r.mac, mac, 6);
    strncpy(r.mac_str, s, sizeof(r.mac_str) - 1);
    r.mac_str[sizeof(r.mac_str) - 1] = '\0';
    r.last_rssi    = 0;
    r.last_seen_ms = millis();
    r.paired_host  = 0;
    r.type         = REMOTE_SPEED;
    r.digits       = 0;
    r.is_this_host = false;

    s_rows.push_back(r);
    if (out != nullptr) {
      *out = &s_rows.back();
    }
    UNLOCK_ROWS();
    return true;
  }
}

void RemoteManager::MacToString(const uint8_t mac[6], char* out18) {
  snprintf(out18, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool RemoteManager::StringToMac(const char* str, uint8_t out_mac[6]) {
  if (!str) {
    return false;
  }
  unsigned int b[6];
  if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
             &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    out_mac[i] = static_cast<uint8_t>(b[i]);
  }
  return true;
}

void RemoteManager::SendDiscover() {
  JsonDocument doc;
  doc["action"] = DISCOVER_REMOTES;
  doc["host"]   = StateManager::getM4ID();

  String payload;
  serializeJson(doc, payload);

  sendMessage(kBroadcastAddress,
              BROADCAST,
              DISCOVER_REMOTES,
              payload,
              HIGH_PRIORITY);
}

bool RemoteManager::SendConnect(const uint8_t mac[6]) {
  char mac_s[18];
  MacToString(mac, mac_s);

  JsonDocument doc;
  doc["action"] = REMOTE_CONTROL;
  doc["op"]     = OPERATION_CONNECT;
  doc["target"] = mac_s;
  doc["host"]   = StateManager::getM4ID();

  String payload;
  serializeJson(doc, payload);

  return sendMessage(mac,
                     COMMAND,
                     REMOTE_CONTROL,
                     payload,
                     HIGH_PRIORITY) == ESP_OK;
}

bool RemoteManager::SendDisconnect(const uint8_t mac[6]) {
  char mac_s[18];
  MacToString(mac, mac_s);

  JsonDocument doc;
  doc["action"] = REMOTE_CONTROL;
  doc["op"]     = OPERATION_DISCONNECT;
  doc["target"] = mac_s;
  doc["host"]   = StateManager::getM4ID();

  String payload;
  serializeJson(doc, payload);

  return sendMessage(mac,
                     COMMAND,
                     REMOTE_CONTROL,
                     payload,
                     HIGH_PRIORITY) == ESP_OK;
}

void RemoteManager::MaybeSendValues() {
  // Do not broadcast remote values while pairing
  if (s_pairing_on) {
    return;
  }

  const uint32_t now = millis();
  if (now - s_last_values_ms < kValuesEveryMs) {
    return;
  }
  s_last_values_ms = now;

  JsonDocument doc;
  doc["action"]   = SEND_REMOTE_VALUE;
  doc["host"]     = StateManager::getM4ID();
  doc["speed"]    = s_cur_speed;
  doc["rpm"]      = s_cur_rpm;
  doc["distance"] = s_cur_dist;
  doc["isMax"]    = s_cur_ismax;
  doc["isSafety"] = s_cur_safety;  // keeping your current field spelling

  String payload;
  serializeJson(doc, payload);

  LOGD("Broadcasting remote values: speed=%.1f rpm=%.1f dist=%.1f isMax=%d isSafety=%d", 
       s_cur_speed, s_cur_rpm, s_cur_dist, s_cur_ismax, s_cur_safety);

  sendMessage(kBroadcastAddress,
              BROADCAST,
              SEND_REMOTE_VALUE,
              payload,
              HIGH_PRIORITY);
}

void RemoteManager::UpdateStatusLabel() {
  if (!s_status_label || !s_status_spinner) {
    return;
  }

  bool labelHidden   = lv_obj_has_flag(s_status_label,   LV_OBJ_FLAG_HIDDEN);
  bool spinnerHidden = lv_obj_has_flag(s_status_spinner, LV_OBJ_FLAG_HIDDEN);

  if (s_pairing_on) {
    if (labelHidden || spinnerHidden) {
      lv_label_set_text(s_status_label, "Searching");
      lv_obj_clear_flag(s_status_label,   LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(s_status_spinner, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_color(
          s_status_label, lv_color_hex(0x1E88E5), 0);
    }
  } else {
    if (!labelHidden || !spinnerHidden) {
      lv_label_set_text(s_status_label, "");
      lv_obj_set_style_text_color(
          s_status_label, lv_color_hex(0x808080), 0);
      lv_obj_add_flag(s_status_label,   LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(s_status_spinner, LV_OBJ_FLAG_HIDDEN);
    }
  }
}
