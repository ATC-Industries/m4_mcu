#ifndef INCLUDE_REMOTES_REMOTEMANAGER_H_
#define INCLUDE_REMOTES_REMOTEMANAGER_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "M4MessageStruct.h"

struct _lv_obj_t;
typedef _lv_obj_t lv_obj_t;

// status label bits
static inline lv_obj_t* status_label_ = nullptr;      // existing uic_Settings1LabelRemoteSearching
static inline lv_obj_t* status_spinner_ = nullptr;    // existing uic_Settings1SpinnerRemoteSearchingSpinner
static inline lv_obj_t* status_row_ = nullptr;        // container for the trio


struct RemoteRow {
  uint8_t mac[6];
  char mac_str[18];
  int last_rssi;
  uint32_t last_seen_ms;
  int paired_host;           // 0 if unpaired
  remote_role type;          // 1 speed, 2 rpm, 3 distance, 4 safety
  uint8_t digits;            // 3/4/5 if reported
  bool is_this_host;         // paired_host == our M4ID
};

class RemoteManager {
 public:
  static void Init();

  // Pairing control
  static void EnterPairing();
  static void ExitPairing();
  static bool IsPairing();

  // Call EVERY loop. Handles both pairing discovery and 5 Hz runtime broadcast.
  static void Tick();

  // Pairing response handler (call from onMessageReceived when action == PAIRING_REMOTE_RESPONSE)
  static void OnPairingRemoteResponse(const uint8_t* sender_mac,
                                      int rssi,
                                      const JsonDocument& doc);

  // Connect / disconnect one remote by MAC string "AA:BB:CC:DD:EE:FF"
  static bool ConnectRemote(const char* mac_str);
  static bool DisconnectRemote(const char* mac_str);

  // Telemetry setters. Call these whenever values change.
  static void SetTelemetry(float speed, float rpm, float distance);
  static void SetIsMax(bool isMaxNow);     // true only during your 1s “max blast”
  static void SetSafety(bool isSafetyNow); // true only when sign green


  // UI
  static void SetListContainer(lv_obj_t* container);
  static void RefreshListUI();  // only updates while pairing is ON

  static void SetTableContainer(lv_obj_t* parent);

  static const std::vector<RemoteRow>& GetRemotes();

 private:
  static bool FindOrAddByMac(const uint8_t mac[6], RemoteRow** out);
  static void MacToString(const uint8_t mac[6], char* out18);
  static bool StringToMac(const char* str, uint8_t out_mac[6]);

  static void SendDiscover();                    // BROADCAST + DISCOVER_REMOTES
  static bool SendConnect(const uint8_t mac[6]); // COMMAND + REMOTE_CONTROL with op=OPERATION_CONNECT
  static bool SendDisconnect(const uint8_t mac[6]);// COMMAND + REMOTE_CONTROL with op=OPERATION_DISCONNECT
  static void MaybeSendValues();                 // throttled 5 Hz broadcast

 private:
  static inline std::vector<RemoteRow> rows_;
  static inline bool pairing_on_ = false;

  // Cadence timers
  static inline uint32_t last_discover_ms_ = 0;
  static inline uint32_t last_purge_ms_    = 0;
  static inline uint32_t last_values_ms_   = 0;

  // Telemetry cache (sent at 5 Hz)
  static inline float cur_speed_ = 0.f;
  static inline float cur_rpm_   = 0.f;
  static inline float cur_dist_  = 0.f;
  static inline bool  cur_ismax_ = false;
  static inline bool  cur_safety_ = false;

  static inline lv_obj_t* table_container_ = nullptr;
  static inline bool remotes_dirty_ = false;

  // status label + simple dot animation
  static inline lv_obj_t* status_label_ = nullptr;
  static inline uint32_t last_status_ms_ = 0;

  static void UpdateStatusLabel();
};

#endif  // INCLUDE_REMOTES_REMOTEMANAGER_H_
