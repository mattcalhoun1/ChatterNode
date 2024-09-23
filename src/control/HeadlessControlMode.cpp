#include "HeadlessControlMode.h"

// these methods need converted to callback approach
uint8_t HeadlessControlMode::promptForPassword (char* passwordBuffer, uint8_t maxPasswordLength) {
    Logger::info("prompt for password!", LogAppControl);
    return 0;
}

void HeadlessControlMode::promptFactoryReset () {
    Logger::info("Prompt for factory reset!", LogAppControl);
}

bool HeadlessControlMode::promptYesNo (const char* message) {
    Logger::info("Prompt for yes/no!", LogAppControl);
    return true;
}

void HeadlessControlMode::showBusy (const char* busyTitle, const char* busyDescription, const char* status, bool cancellable) {
}

void HeadlessControlMode::updateBusyStatus (const char* status) {
   // lv_label_set_text_fmt(ui_lblBusyInfiniteStatus, "%s", status);
}

void HeadlessControlMode::resetStorageProgress () {
    // later
}

void HeadlessControlMode::updateStorageProgress (float pct){
    if (controlModeInitializing) {
        // the last 25% is storage
        //lv_bar_set_value(ui_barStartupProgress, 75 + (pct * 25), LV_ANIM_ON);
    }
}



