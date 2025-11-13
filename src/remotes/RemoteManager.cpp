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

static portMUX_TYPE s_rows_mux = portMUX_INITIALIZER_UNLOCKED;
#define LOCK_ROWS()   portENTER_CRITICAL(&s_rows_mux)
#define UNLOCK_ROWS() portEXIT_CRITICAL(&s_rows_mux)

static volatile bool s_needs_ui_refresh = false;

// Your existing sender
extern esp_err_t sendMessage(const uint8_t *peerAddress,
                             msg_type messageType,
                             action_type messageAction,
                             String payload,
                             priority messagePriority);

// Cadences
static constexpr uint32_t kDiscoverEveryMs = 500;   // pairing discover
static constexpr uint32_t kRowStaleMs      = 5000;  // remove dead entries
static constexpr uint32_t kPurgeEveryMs    = 1000;  // clean list
static constexpr uint32_t kValuesEveryMs   = 200;   // 5 Hz runtime broadcast

void RemoteManager::Init() {
  rows_.clear();
  pairing_on_ = false;
  last_discover_ms_ = last_purge_ms_ = last_values_ms_ = 0;
  cur_speed_ = cur_rpm_ = cur_dist_ = 0.f;
  cur_ismax_ = false;
  cur_safety_ = false;
  table_container_ = nullptr;

  status_label_   = uic_Settings1LabelRemoteSearching;
  status_spinner_ = uic_Settings1SpinnerRemoteSearchingSpinner;

  if (status_label_)   lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);
  if (status_spinner_) lv_obj_add_flag(status_spinner_, LV_OBJ_FLAG_HIDDEN);

  UpdateStatusLabel();
}


void RemoteManager::EnterPairing() {
  pairing_on_ = true;
  last_discover_ms_ = 0;
  last_purge_ms_ = 0;
  rows_.clear();
  RefreshListUI();
  LOGI("[RemoteManager] Pairing ON");
  UpdateStatusLabel();
}

void RemoteManager::ExitPairing() {
  pairing_on_ = false;
  LOGI("[RemoteManager] Pairing OFF");
  // Leave table content frozen until next pairing
  UpdateStatusLabel();
}

bool RemoteManager::IsPairing() { return pairing_on_; }

void RemoteManager::Tick() {
  const uint32_t now = millis();

  if (pairing_on_) {
    if (now - last_discover_ms_ >= kDiscoverEveryMs) {
      last_discover_ms_ = now;
      SendDiscover();
    }
    if (now - last_purge_ms_ >= kPurgeEveryMs) {
      last_purge_ms_ = now;
      bool changed = false;

      LOCK_ROWS();
      for (size_t i = 0; i < rows_.size();) {
        if (now - rows_[i].last_seen_ms > kRowStaleMs) {
          rows_.erase(rows_.begin() + i);
          changed = true;
        } else {
          ++i;
        }
      }
      UNLOCK_ROWS();

      if (changed) s_needs_ui_refresh = true;
    }
  }

  if (remotes_dirty_) {
    remotes_dirty_ = false;
    RefreshListUI();
  }

  // If any RX updated the list, rebuild UI here
  if (pairing_on_ && s_needs_ui_refresh) {
    s_needs_ui_refresh = false;
    RefreshListUI();
  }

  // 5 Hz telemetry
  MaybeSendValues();
}


void RemoteManager::OnPairingRemoteResponse(const uint8_t* sender_mac,
                                            int rssi,
                                            const JsonDocument& doc) {
  if (!pairing_on_) return;

  LOCK_ROWS();
  RemoteRow* rr = nullptr;
  if (!FindOrAddByMac(sender_mac, &rr) || rr == nullptr) {
    UNLOCK_ROWS();
    return;
  }

  rr->last_rssi   = rssi;
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

  // Defer UI work to the main loop
  s_needs_ui_refresh = true;
  remotes_dirty_ = true;
}


bool RemoteManager::ConnectRemote(const char* mac_str) {
  uint8_t mac[6];
  if (!StringToMac(mac_str, mac)) return false;
  bool ok = SendConnect(mac);
  if (!ok) return false;

  // optimistic UI
  LOCK_ROWS();
  for (auto& r : rows_) {
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
  if (!StringToMac(mac_str, mac)) return false;
  bool ok = SendDisconnect(mac);
  if (!ok) return false;

  LOCK_ROWS();
  for (auto& r : rows_) {
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

void RemoteManager::SetTelemetry(float speed, float rpm, float distance) {
  cur_speed_ = speed;
  cur_rpm_   = rpm;
  cur_dist_  = distance;
}

void RemoteManager::SetIsMax(bool isMaxNow) {
  cur_ismax_ = isMaxNow;
}

// TODO: Set when cahnging safety Status
void RemoteManager::SetSafety(bool isSafetyNow) {
  cur_safety_ = isSafetyNow;
}

void RemoteManager::SetListContainer(lv_obj_t* container) {
  table_container_ = container;
  if (pairing_on_) RefreshListUI();
}

void RemoteManager::RefreshListUI() {
  if (!pairing_on_ || !table_container_) return;

  // snapshot rows_ under lock
  std::vector<RemoteRow> snapshot;
  {
    LOCK_ROWS();
    snapshot = rows_;
    UNLOCK_ROWS();  // important
  }

  // Clear old rows once before rebuilding
  lv_obj_clean(table_container_);

  for (const auto& r : snapshot) {
    lv_obj_t* row = lv_obj_create(table_container_);
    if (!row) {
      LOGE("[RemoteManager] lv_obj_create failed");
      return;
    }

    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_obj_set_style_pad_column(row, 8, 0);

    lv_obj_t* mac = lv_label_create(row);
    lv_label_set_text_fmt(mac, "%s", r.mac_str);

    lv_obj_t* type = lv_label_create(row);
    const char* t =
      (r.type == REMOTE_SPEED)        ? "Speed" :
      (r.type == REMOTE_RPM)          ? "RPM" :
      (r.type == REMOTE_DISTANCE)     ? "Distance" :
      (r.type == REMOTE_SAFETY_LIGHT) ? "Safety" : "?";
    lv_label_set_text_fmt(type, "type:%s", t);

    if (r.type != REMOTE_SAFETY_LIGHT) {
      lv_obj_t* digs = lv_label_create(row);
      if (r.digits == 3 || r.digits == 4 || r.digits == 5) {
        lv_label_set_text_fmt(digs, "digits:%u", r.digits);
      } else {
        lv_label_set_text(digs, "digits:-");
      }
    }

    lv_obj_t* host = lv_label_create(row);
    if (r.paired_host > 0) {
      lv_label_set_text_fmt(host, "host:%d%s",
                            r.paired_host,
                            r.is_this_host ? " (this)" : "");
      lv_obj_set_style_text_color(
        host,
        lv_color_hex(r.is_this_host ? 0x1FA709 : 0xA70909),
        0);
    } else {
      lv_label_set_text(host, "host:-");
    }

    // hidden MAC
    lv_obj_t* mac_hidden = lv_label_create(row);
    lv_obj_add_flag(mac_hidden, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(mac_hidden, r.mac_str);

    // connect / disconnect button
    lv_obj_t* btn = lv_btn_create(row);
    lv_obj_t* lbl = lv_label_create(btn);
    bool connected_to_us = r.is_this_host;
    lv_label_set_text(lbl, connected_to_us ? "Disconnect" : "Connect");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(
      btn,
      [](lv_event_t* e) {
        lv_obj_t* btn = lv_event_get_target(e);
        lv_obj_t* row = lv_obj_get_parent(btn);

        lv_obj_t* mac_candidate = nullptr;
        const uint32_t cnt = lv_obj_get_child_cnt(row);
        for (uint32_t i = 0; i < cnt; ++i) {
          lv_obj_t* ch = lv_obj_get_child(row, i);
          if (lv_obj_has_flag(ch, LV_OBJ_FLAG_HIDDEN)) {
            mac_candidate = ch;
            break;
          }
        }
        if (!mac_candidate) return;
        const char* mac = lv_label_get_text(mac_candidate);

        lv_obj_t* lbl = lv_obj_get_child(btn, 0);
        const char* txt = lv_label_get_text(lbl);
        if (strcmp(txt, "Connect") == 0) {
          RemoteManager::ConnectRemote(mac);
        } else {
          RemoteManager::DisconnectRemote(mac);
        }
      },
      LV_EVENT_CLICKED,
      nullptr);
  }
}




const std::vector<RemoteRow>& RemoteManager::GetRemotes() { return rows_; }

// ---------- private helpers ----------

bool RemoteManager::FindOrAddByMac(const uint8_t mac[6], RemoteRow** out) {
  char s[18];
  MacToString(mac, s);

  LOCK_ROWS();
  for (auto& r : rows_) {
    if (strncmp(r.mac_str, s, sizeof(r.mac_str)) == 0) {
      if (out) *out = &r;
      UNLOCK_ROWS();
      return true;
    }
  }
  RemoteRow r{};
  memcpy(r.mac, mac, 6);
  strncpy(r.mac_str, s, sizeof(r.mac_str) - 1);
  r.mac_str[sizeof(r.mac_str) - 1] = '\0';
  r.last_rssi = 0;
  r.last_seen_ms = millis();
  r.paired_host = 0;
  r.type = REMOTE_SPEED;
  r.digits = 0;
  r.is_this_host = false;
  rows_.push_back(r);
  if (out) *out = &rows_.back();
  UNLOCK_ROWS();
  return true;
}


void RemoteManager::MacToString(const uint8_t mac[6], char* out18) {
  snprintf(out18, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool RemoteManager::StringToMac(const char* str, uint8_t out_mac[6]) {
  if (!str) return false;
  unsigned int b[6];
  if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
             &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) out_mac[i] = static_cast<uint8_t>(b[i]);
  return true;
}

void RemoteManager::SendDiscover() {
  JsonDocument doc;;
  doc["action"] = DISCOVER_REMOTES;
  doc["host"]   = StateManager::getM4ID();

  String payload;
  serializeJson(doc, payload);

  sendMessage(kBroadcastAddress, BROADCAST, DISCOVER_REMOTES, payload, HIGH_PRIORITY);
}

bool RemoteManager::SendConnect(const uint8_t mac[6]) {
  char mac_s[18]; MacToString(mac, mac_s);

  JsonDocument doc;;
  doc["action"] = REMOTE_CONTROL;
  doc["op"]     = OPERATION_CONNECT;
  doc["target"] = mac_s;
  doc["host"]   = StateManager::getM4ID();

  String payload; serializeJson(doc, payload);

  return sendMessage(mac, COMMAND, REMOTE_CONTROL, payload, HIGH_PRIORITY) == ESP_OK;
}

bool RemoteManager::SendDisconnect(const uint8_t mac[6]) {
  char mac_s[18]; MacToString(mac, mac_s);

  JsonDocument doc;;
  doc["action"] = REMOTE_CONTROL;
  doc["op"]     = OPERATION_DISCONNECT;
  doc["target"] = mac_s;
  doc["host"]   = StateManager::getM4ID();

  String payload; serializeJson(doc, payload);

  return sendMessage(mac, COMMAND, REMOTE_CONTROL, payload, HIGH_PRIORITY) == ESP_OK;
}

void RemoteManager::MaybeSendValues() {
  // Do not broadcast remote values while pairing
  if (pairing_on_) {
    return;
  }

  const uint32_t now = millis();
  if (now - last_values_ms_ < kValuesEveryMs) return;
  last_values_ms_ = now;

  JsonDocument doc;;
  doc["action"]   = SEND_REMOTE_VALUE;
  doc["host"]     = StateManager::getM4ID();
  doc["speed"]    = cur_speed_;
  doc["rpm"]      = cur_rpm_;
  doc["distance"] = cur_dist_;
  doc["isMax"]    = cur_ismax_;
  doc["isSafery"] = cur_safety_;

  String payload; serializeJson(doc, payload);

  sendMessage(kBroadcastAddress, BROADCAST, SEND_REMOTE_VALUE, payload, HIGH_PRIORITY);
}

void RemoteManager::UpdateStatusLabel() {
  if (!status_label_ || !status_spinner_) return;

  bool labelHidden   = lv_obj_has_flag(status_label_,   LV_OBJ_FLAG_HIDDEN);
  bool spinnerHidden = lv_obj_has_flag(status_spinner_, LV_OBJ_FLAG_HIDDEN);

  if (pairing_on_) {
    // Only do work if they are not already in "searching" state
    if (labelHidden || spinnerHidden) {
      lv_label_set_text(status_label_, "Searching");
      lv_obj_clear_flag(status_label_,   LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_flag(status_spinner_, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_text_color(status_label_, lv_color_hex(0x1E88E5), 0);
    }
  } else {
    // Only do work if they are not already hidden
    if (!labelHidden || !spinnerHidden) {
      lv_label_set_text(status_label_, "");
      lv_obj_set_style_text_color(status_label_, lv_color_hex(0x808080), 0);
      lv_obj_add_flag(status_label_,   LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(status_spinner_, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void RemoteManager::SetTableContainer(lv_obj_t* parent) {
  table_container_ = parent;
  if (table_container_) {
    lv_obj_clean(table_container_);
  }
  remotes_dirty_ = true;  // force one rebuild when we come back
}

