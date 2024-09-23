#include "UserEvents.h"

UserEvents::UserEvents () {
}

bool UserEvents::queueEvent (ChatterUserEvent evt) {
    // bump any existing events out, older ones will drop off edge if queue is full
    if (numEvents > 0) {
        for (uint8_t q = max(numEvents, (uint8_t)MAX_QUEUED_EVENTS); q > 0; q--) {
            if (q < MAX_QUEUED_EVENTS) {
                queuedEvents[q] = queuedEvents[q-1];
            }
        }
    }

    queuedEvents[0] = evt;
    numEvents = min(numEvents+1, MAX_QUEUED_EVENTS);

    return true;
}

ChatterUserEvent UserEvents::dequeueNextEvent () {
    ChatterUserEvent thisEvent = UserEventNone;

    if (numEvents > 0) {
        thisEvent = queuedEvents[0];
        // shift events to the left
        for (uint8_t q = 0; q < numEvents; q++) {
            queuedEvents[q] = queuedEvents[q+1];
        }

        numEvents -= 1; // drop num events by 1
    }

    return thisEvent;
}
