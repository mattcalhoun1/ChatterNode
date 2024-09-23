#include "PreferenceHandler.h"
#include "../control/BasicControl.h"
#include "ChatterAll.h"

#ifndef PREFERENCEHANDLERIMPL_H
#define PREFERENCEHANDLERIMPL_H

class PreferenceHandlerImpl : public PreferenceHandler {
    public:
        PreferenceHandlerImpl (Chatter* _chatter, BasicControl* _control) { chatter = _chatter; control = _control; }
        bool isPreferenceEnabled (CommunicatorPreference pref);
        void enablePreference (CommunicatorPreference pref);
        void disablePreference (CommunicatorPreference pref);

    protected:
        Chatter* chatter;
        BasicControl* control;
};

#endif
