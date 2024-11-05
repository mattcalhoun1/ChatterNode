#include <Arduino.h>
#include "../../globals/Globals.h"
#include "ChatterAll.h"
#include "../../events/CommunicatorEvent.h"
#include "../Backpack.h"

#ifndef RELAYBACKPACK_H
#define RELAYBACKPACK_H

#define RELAY_OUT_PIN 46

class RelayBackpack : public Backpack {
    public:
        RelayBackpack (Chatter* _chatter, ChatStatusCallback* _control) : Backpack(_chatter, _control) {}

        bool handleUserEvent (CommunicatorEventType eventType);
        bool handleMessage (const uint8_t* message, int messageSize, const char* senderId, const char* recipientId);
        bool init ();
        bool isRunning ();
        const char* getName () { return "Relay"; }
        BackpackType getType () { return BackpackTypeRelay; }


    protected:
        void triggerRelay ();

        bool running = false;
        bool remoteEnabled = true;
        bool autoEnabled = true;
};

#endif