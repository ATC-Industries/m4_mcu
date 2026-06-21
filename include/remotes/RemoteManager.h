#ifndef INCLUDE_REMOTES_REMOTEMANAGER_H_
#define INCLUDE_REMOTES_REMOTEMANAGER_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "M4MessageStruct.h"

struct _lv_obj_t;
typedef _lv_obj_t lv_obj_t;

struct RemoteRow {
  uint8_t mac[6];
  char mac_str[18];
  int last_rssi;
  uint32_t last_seen_ms;
  int paired_host;    // 0 if unpaired
  remote_role type;   // 1 speed, 2 rpm, 3 distance, 4 safety
  uint8_t digits;     // 3/4/5 if reported
  bool is_this_host;  // paired_host == our M4ID
};

class RemoteManager {
 public:
  static void Init();

  // Pairing control
  static void EnterPairing();
  static void ExitPairing();
  static bool IsPairing();

  // Call from loop()
  static void Tick();

  // Called from onMessageReceived when action == PAIRING_REMOTE_RESPONSE
  static void OnPairingRemoteResponse(const uint8_t* sender_mac,
                                      int rssi,
                                      const JsonDocument& doc);

  // Connect / disconnect by MAC string "AA:BB:CC:DD:EE:FF"
  static bool ConnectRemote(const char* mac_str);
  static bool DisconnectRemote(const char* mac_str);

  // Telemetry setters
  static void SetTelemetry(float speed, float rpm, float distance);
  static void SetIsMax(bool isMaxNow);
  static void SetSafety(bool isSafetyNow);

  // Hooked from UI when the Settings1 screen is shown / hidden
  static void SetTableContainer(lv_obj_t* parent);

  // Called by DeviceRowActionButtonCB1..6 with row_index 0..5
  static void HandleRowButtonClicked(int row_index);

 private:
  static void RefreshListUI();   // only updates while pairing is on
  static void ClearAllRowsUI();
  static void ClearAllRemotes();

  static bool FindOrAddByMac(const uint8_t mac[6], RemoteRow** out);
  static void MacToString(const uint8_t mac[6], char* out18);
  static bool StringToMac(const char* str, uint8_t out_mac[6]);

  static void SendDiscover();
  static bool SendConnect(const uint8_t mac[6]);
  static bool SendDisconnect(const uint8_t mac[6]);
  static void MaybeSendValues();

  static void UpdateStatusLabel();
};

#endif  // INCLUDE_REMOTES_REMOTEMANAGER_H_
