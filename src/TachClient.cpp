#include "TachClient.h"
#include <ArduinoJson.h>
#include <lvgl/lvgl.h>
#include "ui/ui.h"
#include <climits>

namespace Tach {
    State state = {
        .isTSSConnected = false,
        .pairedTSSAddress = {0},
        .pairedTSSRSSI = INT_MIN
    };
}

void Tach::pairTSS(lv_event_t *e) {
    LOGI("[Tach] Starting TSS pairing process");

    state.pairedTSSRSSI = INT_MIN;
    memset(state.pairedTSSAddress, 0, 6);
    state.isTSSConnected = false;

    // Start pairing mode
    state.isPairing = true;
    state.pairingStartedMillis = millis();
    state.lastPairingBroadcastMillis = 0;  // Force immediate first broadcast
    state.pairingBroadcastCount = 0;

    LOGI("[Tach] Pairing mode activated - will broadcast for 5 seconds");
}

// Call this from your main loop or a timer
void Tach::updatePairing() {
  if (!state.isPairing) return;

    uint32_t now = millis();
    
    if (now - state.pairingStartedMillis > 500) {
        state.isPairing = false;
        
        if (state.pairedTSSRSSI == INT_MIN) {
            LOGW("[Tach] Pairing timeout - no TSS found");
            state.isTSSConnected = false;
        } else {
            LOGI("[Tach] Pairing complete - locked onto TSS at %02X:%02X:%02X:%02X:%02X:%02X with RSSI: %d",
                 state.pairedTSSAddress[0], state.pairedTSSAddress[1], state.pairedTSSAddress[2],
                 state.pairedTSSAddress[3], state.pairedTSSAddress[4], state.pairedTSSAddress[5],
                 state.pairedTSSRSSI);
            
            state.isTSSConnected = true; 
            
            requestRPM();
        }
        return;
    }

    // Send broadcast every 500ms during pairing
    if (now - state.lastPairingBroadcastMillis >= 100) {
        state.lastPairingBroadcastMillis = now;
        state.pairingBroadcastCount++;

        JsonDocument doc;
        doc["action"] = SEND_PROXIMITY;
        doc["reqDevice"] = M4_TACH_SENSOR;

        String payload;
        doc.shrinkToFit();
        serializeJson(doc, payload);

        //LOGD("[Tach] Pairing broadcast #%d: %s", state.pairingBroadcastCount, payload.c_str());

        uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        sendMessage(broadcastAddress, BROADCAST, SEND_PROXIMITY, payload, HIGH_PRIORITY);
    }
}

void Tach::requestRPM() {
    if (!state.isTSSConnected) {
        LOGW("[Tach] Cannot request RPM - not connected to TSS");
        return;
    }

    JsonDocument doc;
    doc["action"] = SEND_RPM;

    String payload;
    doc.shrinkToFit();
    serializeJson(doc, payload);

    //LOGD("[Tach] Requesting RPM from locked TSS");

    // Send to the paired TSS address
    sendMessage(state.pairedTSSAddress, REQUEST, SEND_RPM, "{}", HIGH_PRIORITY);
}

// Call this when you receive a PAIRING_RESPONSE
void Tach::onPairingResponse(uint8_t *senderAddress, int rssi) {
    if (!state.isPairing) {
        LOGD("[Tach] Ignoring pairing response - already locked to a TSS");
        return;
    }

    if (rssi > state.pairedTSSRSSI) {
        state.pairedTSSRSSI = rssi;
        memcpy(state.pairedTSSAddress, senderAddress, 6);
        
        LOGI("[Tach] Better TSS candidate - RSSI: %d, Address: %02X:%02X:%02X:%02X:%02X:%02X",
             rssi,
             senderAddress[0], senderAddress[1], senderAddress[2],
             senderAddress[3], senderAddress[4], senderAddress[5]);
    }
}

void Tach::update() {
    // Handle pairing broadcasts
    updatePairing();
    
    // Request RPM from connected TSS every 250ms
    if (state.isTSSConnected) {
        static uint32_t lastRPMRequest = 0;
        uint32_t now = millis();
        
        if (now - lastRPMRequest >= 250) {
            lastRPMRequest = now;
            requestRPM();
        }
    }
}