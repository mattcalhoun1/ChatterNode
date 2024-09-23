#ifndef USEREVENTS_H
#define USEREVENTS_H

#include <Arduino.h>
#include "ChatterUserEvent.h"

#define MAX_QUEUED_EVENTS 2

// not threadsafe, but doesn't need to be for base
class UserEvents {
    public:
        UserEvents ();
        bool queueEvent (ChatterUserEvent evt);
        ChatterUserEvent dequeueNextEvent ();

    protected:
        uint8_t numEvents = 0;
        ChatterUserEvent queuedEvents[MAX_QUEUED_EVENTS];

};

#endif