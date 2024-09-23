#include "ChatterAll.h"
#include "ControlMode.h"

#ifndef HEADLESSCONTROLMODE_H
#define HEADLESSCONTROLMODE_H

class HeadlessControlMode : public ControlMode {
    public:
        HeadlessControlMode (DeviceType _deviceType, RTClockBase* _rtc, CallbackRegistry* _callbackRegistry, uint8_t _sdPin, SPIClass & _sdSpiClass) : ControlMode(_deviceType, _rtc, _callbackRegistry, _sdPin, _sdSpiClass) {}

        // these methods need converted to callback approach
        uint8_t promptForPassword (char* passwordBuffer, uint8_t maxPasswordLength);
        void promptFactoryReset ();
        bool promptYesNo (const char* message);

        void showBusy (const char* busyTitle, const char* busyDescription, const char* status, bool cancellable);
        void updateBusyStatus (const char* status);

        void resetStorageProgress();
        void updateStorageProgress (float pct);
};

#endif