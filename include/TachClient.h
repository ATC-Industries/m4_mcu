#ifndef TACHCLIENT_H
#define TACHCLIENT_H

#include <Arduino.h>
#include <lvgl/lvgl.h>
#include "M4CommsHelpers.h"
#include "StateManager.h"

namespace Tach {
    struct State {
        bool isTSSConnected;
        
        // Pairing state
        bool isPairing;
        uint32_t pairingStartedMillis;
        uint32_t lastPairingBroadcastMillis;
        int pairingBroadcastCount;
        int lastRssi;
        
        // Connected TSS info
        uint8_t pairedTSSAddress[6];
        int pairedTSSRSSI;
    };

    extern State state;
    
    void pairTSS(lv_event_t* e);
    void update();
    void updatePairing();
    void requestRPM();
    void onPairingResponse(uint8_t *senderAddress, int rssi);
}

#endif // TACHCLIENT_H
