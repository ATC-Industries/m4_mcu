#include "data_export.h"

#include <Arduino.h>
#include <M4CommProtocol.h>
#include <QuickEspNow.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "M4CommsHelpers.h"
#include "StateManager.h"

#define LOG_TAG "DataExport"
#include "Logging.h"

#include "ui/ui.h"

namespace {

constexpr unsigned long EXPORT_TIMEOUT_MS = 180000UL;
constexpr uint16_t HTTP_PORT = 80;
constexpr int QR_SIZE = 140;

WebServer s_server(HTTP_PORT);

DataExportState s_state = DataExportState::IDLE;
bool s_routesConfigured = false;
bool s_espNowPaused = false;

String s_ssid;
String s_csvSnapshot;
String s_apUrl;
String s_filename;
unsigned long s_sessionStartedMs = 0;
unsigned long s_lastRequestMs = 0;
int s_snapshotRowCount = 0;

String csvQuote(const String &value) {
  String escaped = value;
  escaped.replace("\"", "\"\"");
  return "\"" + escaped + "\"";
}

String buildSessionSsid() {
  const uint64_t efuseMac = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X",
           static_cast<uint8_t>((efuseMac >> 16) & 0xFF),
           static_cast<uint8_t>((efuseMac >> 8) & 0xFF),
           static_cast<uint8_t>(efuseMac & 0xFF));
  return String("M4-") + suffix;
}

String buildCsvSnapshot() {
  const PullResult *history = StateManager::getPullHistory();
  const int count = StateManager::getPullHistoryCount();

  String csv;
  csv.reserve(128 + count * 96);
  csv += "pull_index,driverName,driverNumber,className,classWeight,maxSpeedMPH,maxDistanceFeet,maxRPM,uptime_s\n";

  for (int i = 0; i < count; ++i) {
    const PullResult &pull = history[i];
    csv += String(i + 1);
    csv += ",";
    csv += csvQuote(pull.driverName);
    csv += ",";
    csv += String(pull.driverNumber);
    csv += ",";
    csv += csvQuote(pull.className);
    csv += ",";
    csv += String(pull.classWeight);
    csv += ",";
    csv += String(pull.maxSpeedMPH, 2);
    csv += ",";
    csv += String(pull.maxDistanceFeet, 2);
    csv += ",";
    csv += String(pull.maxRPM, 1);
    csv += ",";
    csv += String(pull.timestamp);
    csv += "\n";
  }

  s_snapshotRowCount = count;
  return csv;
}

String buildLandingPage() {
  String html;
  html.reserve(2200);
  html += "<!doctype html><html><head><meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<title>M4 Data Export</title>";
  html += "<style>";
  html += "body{margin:0;font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,sans-serif;";
  html += "background:#f3f6fa;color:#102033;display:flex;min-height:100vh;align-items:center;justify-content:center;padding:24px;}";
  html += ".card{max-width:560px;width:100%;background:#fff;border-radius:18px;padding:28px;";
  html += "box-shadow:0 18px 50px rgba(16,32,51,.12);}";
  html += "h1{margin:0 0 10px;font-size:32px;line-height:1.1;}";
  html += "p{font-size:18px;line-height:1.45;margin:0 0 16px;}";
  html += ".meta{font-size:14px;color:#4b5d73;margin:0 0 20px;}";
  html += ".btn{display:block;background:#0b6bcb;color:#fff;text-decoration:none;text-align:center;";
  html += "padding:16px 22px;border-radius:14px;font-weight:700;font-size:18px;border:0;width:100%;box-sizing:border-box;}";
  html += ".note{margin-top:18px;font-size:14px;line-height:1.45;color:#607286;}";
  html += "</style></head><body><main class=\"card\">";
  html += "<h1>M4 Data Export</h1>";
  html += "<p>Download all pull history stored on this device as a CSV.</p>";
  html += "<div class=\"meta\">Rows: ";
  html += String(s_snapshotRowCount);
  html += "<br>Snapshot uptime: ";
  html += String(s_sessionStartedMs / 1000);
  html += " s</div>";
  html += "<a class=\"btn\" href=\"/data.csv\">Download CSV</a>";
  html += "<div class=\"note\">If the download does not start, reload this page in your browser and try again. This network closes automatically when the export session ends.</div>";
  html += "</main></body></html>";
  return html;
}

void noteHttpActivity() {
  s_lastRequestMs = millis();
}

void handleRoot() {
  noteHttpActivity();
  s_server.send(200, "text/html", buildLandingPage());
}

void handleCsv() {
  noteHttpActivity();
  s_server.sendHeader("Content-Disposition", "attachment; filename=\"" + s_filename + "\"");
  s_server.send(200, "text/csv", s_csvSnapshot);
}

void configureRoutesIfNeeded() {
  if (s_routesConfigured) {
    return;
  }

  s_server.on("/", HTTP_GET, handleRoot);
  s_server.on("/data.csv", HTTP_GET, handleCsv);
  s_server.onNotFound([]() {
    noteHttpActivity();
    s_server.send(404, "text/plain", "Not found");
  });
  s_routesConfigured = true;
}

String currentApUrl() {
  IPAddress ip = WiFi.softAPIP();
  if (ip == INADDR_NONE || ip[0] == 0) {
    return String("http://192.168.4.1");
  }
  return String("http://") + ip.toString();
}

void restoreEspNow() {
  initCommProtocol();
  quickEspNow.onDataRcvd(onMessageReceived);
  s_espNowPaused = false;
}

void refreshPanelIfPresent() {
  if (ui_ExportScreenPanelinstructionPanel != nullptr) {
    data_export_build_screen(ui_ExportScreenPanelinstructionPanel);
  }
}

void shutdownSession() {
  s_state = DataExportState::SHUTDOWN;

  s_server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  if (s_espNowPaused) {
    restoreEspNow();
  }

  s_apUrl = String("http://192.168.4.1");
  s_state = DataExportState::IDLE;
  refreshPanelIfPresent();
  LOGI("Export session stopped");
}

bool beginApSession() {
  s_ssid = buildSessionSsid();
  s_csvSnapshot = buildCsvSnapshot();
  s_sessionStartedMs = millis();
  s_lastRequestMs = s_sessionStartedMs;
  s_filename = "m4_data_" + String(s_sessionStartedMs / 1000) + ".csv";

  quickEspNow.stop();
  s_espNowPaused = true;

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(s_ssid.c_str())) {
    LOGE("Failed to start SoftAP");
    WiFi.mode(WIFI_OFF);
    restoreEspNow();
    s_csvSnapshot = "";
    s_ssid = "";
    s_filename = "m4_data.csv";
    return false;
  }

  configureRoutesIfNeeded();
  s_apUrl = currentApUrl();
  s_server.begin();
  return true;
}

void addInfoLabel(lv_obj_t *parent, const char *text, lv_align_t align, lv_text_align_t textAlign) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, lv_pct(100));
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(label, textAlign, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0x183153), 0);
  lv_obj_align(label, align, 0, 0);
}

void onStartExportButtonClicked(lv_event_t *e) {
  (void)e;
  data_export_start();
}

}  // namespace

DataExportState data_export_get_state() {
  return s_state;
}

void data_export_start() {
  if (s_state != DataExportState::IDLE) {
    LOGD("Ignoring export start while state=%d", static_cast<int>(s_state));
    return;
  }

  s_state = DataExportState::STARTING_AP;
  if (!beginApSession()) {
    s_state = DataExportState::IDLE;
    refreshPanelIfPresent();
    return;
  }

  if (WiFi.softAPIP()[0] != 0) {
    s_apUrl = currentApUrl();
    s_state = DataExportState::READY;
  }

  refreshPanelIfPresent();
  LOGI("Export session started: SSID=%s URL=%s", s_ssid.c_str(), s_apUrl.c_str());
}

void data_export_stop() {
  if (s_state == DataExportState::IDLE) {
    return;
  }

  shutdownSession();
}

void data_export_loop() {
  if (s_state == DataExportState::STARTING_AP) {
    if (WiFi.softAPIP()[0] != 0) {
      s_apUrl = currentApUrl();
      s_state = DataExportState::READY;
      refreshPanelIfPresent();
    }
    return;
  }

  if (s_state != DataExportState::READY) {
    return;
  }

  s_server.handleClient();

  if (millis() - s_lastRequestMs > EXPORT_TIMEOUT_MS) {
    LOGI("Export session timed out");
    shutdownSession();
  }
}

void data_export_build_screen(lv_obj_t *panel) {
  if (panel == nullptr) {
    return;
  }

  lv_obj_clean(panel);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_left(panel, 20, 0);
  lv_obj_set_style_pad_right(panel, 20, 0);
  lv_obj_set_style_pad_top(panel, 18, 0);
  lv_obj_set_style_pad_bottom(panel, 18, 0);
  lv_obj_set_style_pad_row(panel, 12, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  if (s_state != DataExportState::READY && s_state != DataExportState::STARTING_AP) {
    addInfoLabel(panel, "Export session is not active.", LV_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER);
    addInfoLabel(panel, "Tap Download Data below to start the access point and QR code session.", LV_ALIGN_CENTER,
                 LV_TEXT_ALIGN_CENTER);

    lv_obj_t *startButton = lv_btn_create(panel);
    lv_obj_clear_flag(startButton, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(startButton, lv_color_hex(0x0B6BCB), 0);
    lv_obj_set_style_bg_opa(startButton, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(startButton, 18, 0);
    lv_obj_set_style_pad_right(startButton, 18, 0);
    lv_obj_set_style_pad_top(startButton, 12, 0);
    lv_obj_set_style_pad_bottom(startButton, 12, 0);
    lv_obj_add_event_cb(startButton, onStartExportButtonClicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *startLabel = lv_label_create(startButton);
    lv_label_set_text(startLabel, "Download Data");
    lv_obj_set_style_text_font(startLabel, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(startLabel, lv_color_white(), 0);
    lv_obj_center(startLabel);
    return;
  }

  lv_obj_t *qrRow = lv_obj_create(panel);
  lv_obj_remove_style_all(qrRow);
  lv_obj_set_width(qrRow, lv_pct(100));
  lv_obj_set_height(qrRow, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(qrRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(qrRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  auto buildQrCard = [&](const char *titleText, const String &payload) {
    lv_obj_t *card = lv_obj_create(qrRow);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(card, QR_SIZE+40);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xF8FBFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xC6D8EC), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *cardTitle = lv_label_create(card);
    lv_label_set_text(cardTitle, titleText);
    lv_obj_set_style_text_font(cardTitle, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_text_color(cardTitle, lv_color_hex(0x102033), 0);

    lv_obj_t *qrFrame = lv_obj_create(card);
    lv_obj_clear_flag(qrFrame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(qrFrame, QR_SIZE + 20, QR_SIZE + 20);
    lv_obj_set_style_bg_color(qrFrame, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(qrFrame, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(qrFrame, lv_color_hex(0xA8B7C7), 0);
    lv_obj_set_style_border_width(qrFrame, 2, 0);
    lv_obj_set_style_radius(qrFrame, 10, 0);
    lv_obj_set_style_pad_all(qrFrame, 10, 0);

    lv_obj_t *qr = lv_qrcode_create(qrFrame, QR_SIZE, lv_color_black(), lv_color_white());
    lv_obj_center(qr);

    if (lv_qrcode_update(qr, payload.c_str(), payload.length()) != LV_RES_OK) {
      lv_obj_del(qr);
      addInfoLabel(qrFrame, "QR unavailable", LV_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER);
    }
  };

  buildQrCard("1. Join This Network", "WIFI:T:nopass;S:" + s_ssid + ";;");
  buildQrCard("2. Visit This Page", s_apUrl);

  addInfoLabel(panel, "1. Scan the left QR code and join the M4 Wi-Fi network.", LV_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER);
  addInfoLabel(panel, "2. Scan the right QR code to open the browser page.", LV_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER);
  addInfoLabel(panel, "3. Tap Download CSV in the browser page.", LV_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER);
  addInfoLabel(panel, "4. Manual fallback: join the network below, then type the page address into your browser.",
               LV_ALIGN_CENTER, LV_TEXT_ALIGN_CENTER);

  lv_obj_t *ssidLabel = lv_label_create(panel);
  lv_label_set_text_fmt(ssidLabel, "Network: %s   Page: %s", s_ssid.c_str(), s_apUrl.c_str());
  lv_obj_set_style_text_font(ssidLabel, LV_FONT_DEFAULT, 0);
  lv_obj_set_style_text_color(ssidLabel, lv_color_hex(0x102033), 0);

  // lv_obj_t *urlLabel = lv_label_create(panel);
  // lv_label_set_text_fmt(urlLabel, "Page: %s", s_apUrl.c_str());
  // lv_obj_set_style_text_font(urlLabel, LV_FONT_DEFAULT, 0);
  // lv_obj_set_style_text_color(urlLabel, lv_color_hex(0x102033), 0);

  // lv_obj_t *metaLabel = lv_label_create(panel);
  // lv_label_set_text_fmt(metaLabel, "Rows: %d", s_snapshotRowCount);
  // lv_obj_set_style_text_font(metaLabel, LV_FONT_DEFAULT, 0);
  // lv_obj_set_style_text_color(metaLabel, lv_color_hex(0x4A607A), 0);
}
