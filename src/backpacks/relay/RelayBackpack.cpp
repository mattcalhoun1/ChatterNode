#include "RelayBackpack.h"

bool RelayBackpack::init () {
    //if (chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpacksEnabled) == 'T') {
    //    if (chatter->getDeviceStore()->getCustomPreference(StoredPrefBackpackRelayEnabled) == 'T') {
            control->updateChatStatus("Init Relay");

            // set the pin for analog out
            pinMode(RELAY_OUT_PIN, OUTPUT);
            digitalWrite(RELAY_OUT_PIN, LOW);
            running = true;
            Logger::warn("Relay Ready!", LogAppControl);

    //    }
    //}

    return running;
}

bool RelayBackpack::isRunning () {
    return running;
}

bool RelayBackpack::handleUserEvent (CommunicatorEventType eventType) {
  switch(eventType) {
    case UserPressActionButton:
        Logger::warn("User Triggered Relay Via Button", LogAppControl);

        triggerRelay();

        return true;
  }

  return false;
}

bool RelayBackpack::handleMessage (const uint8_t* message, int messageSize, const char* senderId, const char* recipientId) {
    // only one option at the moment, return a current image
    if (remoteEnabled) {
        Logger::warn("Relay Triggered Via Message", LogAppControl);
        control->updateChatStatus("* RELAY TRIGGERED *");
        
        triggerRelay();
        return true;
    }
    else {
        Logger::warn("Remote relay requested, but not enabled in settings", LogAppControl);
    }

    return false;
}


void RelayBackpack::triggerRelay() {
    // set high for 5 seconds, then set low
    digitalWrite(RELAY_OUT_PIN, HIGH);

    Logger::warn("Relay On", LogAppControl);

    unsigned long startTime = millis();
    while (millis() - startTime < 5000) {
        delay(100);
    }
    digitalWrite(RELAY_OUT_PIN, LOW);

    Logger::warn("Relay Off", LogAppControl);
}