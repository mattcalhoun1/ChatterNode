#include "ControlLayer.h"

ControlLayer::ControlLayer(CallbackRegistry* _callbackRegistry) {
    globalCallbackRegistry = _callbackRegistry;
    globalCallbackRegistry->addCallback(CallbackChatStatus, this);

    memset(&displayLines[0][0], 0, DISPLAY_LINE_WIDTH*DISPLAY_NUM_LINES);
    display = new SH1106Wire(0x3c);
    if (!display->init()) {
        Logger::error("Display not ready!", LogUi);
        displayRunning = false;
    }
    else {
        display->cls();
        display->display();
        displayRunning = true;
    }

    sprintf(&displayLines[DISPLAY_TITLE_ROW][0], "%s v%s", "ChatterBox", CHATTERBOX_FIRMWARE_VERSION);
    sprintf(&displayLines[DISPLAY_DASHBOARD_ROW][0], "Enc: %s", STRONG_ENCRYPTION_ENABLED ? "STRONG" : "EXPORT");
    sprintf(&displayLines[DISPLAY_STATUS_ROW][0], "Starting Up...");

    updateDisplay();
}

void ControlLayer::setControlMode (ControlMode* _control) {
    control = _control;
}

bool ControlLayer::process (ChatterUserEvent evt) {
    if (!initialized) {
        // do nothing until initialized
        processControlModeNotReady(evt);
        return true;
    }    

    if (status == ControlModeProcessing) {
        Logger::info("Already processing, not re-entrant", LogAppControl);
        return true;
    }
    if (status == ControlModePaused) {
        Logger::debug("Control layer paused", LogAppControl);
    }
    else if (status == ControlModeReady) {
        status = ControlModeProcessing;
    }    

    // this is copied from tdeck implementation, trying to keep them
    // as similar as possible for now
  switch (evt) {
    default:
        if (control != nullptr) {
            if (status == ControlModeReady || status == ControlModeProcessing) {
                rotateDisplay();

                // Only flush storage if screen is locked, so we dont tie up UI
                bool storageFlushed = false;
                if (isMessagingPaused() == false) {
                    storageFlushed = control->flushStorage();
                    if (storageFlushed) {
                        Logger::debug("SD was written", LogAppControl);
                    }
                }

                // dont take up any processing time if a flush was just done
                if (isMessagingPaused() == false && storageFlushed == false) {
                    control->processOneCycle(ControlCycleFull);
                }
            }
            else {
                // must be awaiting license or setup
                processControlModeNotReady(evt);
            }
        }
        else {
            Logger::debug("Awaiting handle to control mode...", LogAppControl);
        }
  }

    if (status == ControlModeProcessing) {
        status = ControlModeReady;
    }

  return true;
}


void ControlLayer::processControlModeNotReady (ChatterUserEvent evt) {
    if (status == ControlStartupUnlicensed) {
        Logger::warn("Device is unlicensed!", LogAppControl);
        while(true) {
            delay(10);
        }
    }
    else if (status == ControlStartupInitializeDevice) {
        Logger::warn("Base will be initialized", LogAppControl);

        char randomAlias[10];
        sprintf(randomAlias, "b.%05d", random(0, 99999));

        // automatically generate a new device identity and cluster
        sprintf(deviceInitializationForm.clusterAlias, "temp");
        sprintf(deviceInitializationForm.numChannels, BASE_DEFAULT_NUM_CHANNELS);
        sprintf(deviceInitializationForm.hopSchedule, BASE_DEFAULT_HOP_SCHEDULE);
        sprintf(deviceInitializationForm.centerFrequency, BASE_DEFAULT_CENTER_FREQ);
        sprintf(deviceInitializationForm.daylightSavings, "F");
        sprintf(deviceInitializationForm.timeZone, "New York");
        sprintf(deviceInitializationForm.deviceAlias, randomAlias);
        sprintf(deviceInitializationForm.passwordProtected, "F");
        deviceInitializationForm.devicePassword[0] = 0;

        ((HeadlessControlMode*)control)->updateBusyStatus("New Base Setup...");
        control->initializeNewDevice(&deviceInitializationForm);

        // restart the device
        ((HeadlessControlMode*)control)->updateBusyStatus("Restarting...");
        delay(500);
        ((HeadlessControlMode*)control)->restartDevice();
    }
}

void ControlLayer::pauseMessagingFor(unsigned long pauseLengthMillis) { 
    // if messaging is already paused for a certain amount of time,
    // we dont' want to reduce the pause
    unsigned long thisPause = millis() + pauseLengthMillis;
    if (thisPause > messagingPausedUntil) {
        messagingPausedUntil = thisPause;
    }
}

void ControlLayer::configureDeviceSettings() {
    ScreenTimeoutValue screenTimeoutSetting = control->getChatter()->getDeviceStore()->getScreenTimeout();
    switch (screenTimeoutSetting) {
        case ScreenTimeout1Min:
            screenTimeout = 60000ul;
            break;
        case ScreenTimeout2Min:
            screenTimeout = 120000ul;
            break;
        case ScreenTimeout5Min:
            screenTimeout = 5ul*60000ul;
            break;
        case ScreenTimeoutNever:
            screenTimeout = 0ul;
            break;
    }
    control->getChatter()->getRtc()->setGpsUpdateFrequency(60000); // only every 60 sec. should become setting
}

bool ControlLayer::joinCluster () {
    updateChatViewStatus("Please Onboard Me!");
    control->joinCluster();
}

ControlModeStatus ControlLayer::initializeNextStep () {
    StartupState thisStartupState;

    switch (status) {
        case ControlModeIdle:
            if (control->initEncryptedStorage() == StartupEncryptedStorageReady) {
                status = ControlStartupEncryptedStorageReady;
            }
            else {
                Logger::error("Init encrypted storage failed!", LogAppControl);
                while (true) {
                    delay(10);
                }
            }
            break;
        case ControlStartupEncryptedStorageReady:
            if (control->initLicense() == StartupLicensed) {
                status = ControlStartupLicensed;
            }
            else {
                Logger::warn("Init device not licensed", LogAppControl);
                status = ControlStartupUnlicensed;
            }
            break;
        case ControlStartupLicensed:
            thisStartupState = control->initDeviceStore();
            if (thisStartupState == StartupDeviceStoreReady) {
                status = ControlStartupDeviceStoreReady;
            }
            else if (thisStartupState == StartupNeedPassword) {
                status = ControlStartupNeedPassword;
            }
            else {
                Logger::error("Init device store failed!", LogAppControl);
            }
            break;
        case ControlStartupNeedPassword:
            ((HeadlessControlMode*)control)->updateBusyStatus("Password protected!");
            Logger::error("Base cannot have password!", LogAppControl);
            while (true) {
                delay(10);
            }

            break;
        case ControlStartupDeviceStoreReady:
            // allow a delay to pass for the ui to refresh properly
            if (isMessagingPaused() == false) {
                if (control->finishDeviceInit() == StartupDeviceInitialized) {
                    if (control->startChatter()) {
                        Logger::debug("chatter layer has started", LogAppControl);

                        configureDeviceSettings();

                        control->setListeningForMessages(true);
                        pauseMessagingFor(2000); // delay to allow init to complete

                        if (control->getChatter()->isRootDevice(control->getChatter()->getDeviceId())) {
                            sprintf(titleLine, "%s [onboard]", control->getChatter()->getDeviceAlias());
                            updateTitle(titleLine);
                        }
                        else {
                            // dont need onboarded, ready to go
                            isClusterRoot = false;
                            sprintf(titleLine, "%s @ %s", control->getChatter()->getDeviceAlias(), control->getChatter()->getClusterAlias());
                            updateTitle(titleLine);
                        }


                        status = ControlModeReady;
                    }
                    else {
                        Logger::error("Failed to start chatter layer!", LogAppControl);

                        // prompt for factory reset
                        status = ControlModeError;
                    }
                }
                else {
                    Logger::warn("Device needs initialized", LogAppControl);
                    status = ControlStartupInitializeDevice;
                }
            }
            break;
    }

    return status;
}

void ControlLayer::messageReceived () {
    updateDisplay("Message Received", DISPLAY_STATUS_ROW);
}

void ControlLayer::updateCacheUsed(float pct) {
    meshCachePct = pct;
    updateDisplay();
}

void ControlLayer::updateChatViewStatus (const char* status) {
    updateDisplay(status, DISPLAY_STATUS_ROW);
}

void ControlLayer::updateChatViewProgress (float progress) {
    generalProgress = progress;
    updateDisplay();
}

void ControlLayer::updateChatViewStatus (uint8_t channelNum, ChatViewStatus newStatus) {
    updateDashboard();
}

void ControlLayer::updateDashboard () {
    uint8_t dashPos = 0;
    Chatter* chatter = control->getChatter();
    for (uint8_t c = 0; c < chatter->getNumChannels(); c++) {
        if (c > 0) {
            displayLines[DISPLAY_DASHBOARD_ROW][dashPos++] = ',';
            displayLines[DISPLAY_DASHBOARD_ROW][dashPos++] = ' ';
            displayLines[DISPLAY_DASHBOARD_ROW][dashPos++] = ' ';
        }
        ChatterChannel* chan = chatter->getChannel(c);
        sprintf(displayRowBuffer, "%s@%s %s", chan->getName(), chan->getConfigName(), getStatusName(chatter->getChatStatus(c)));
        sprintf(&displayLines[DISPLAY_DASHBOARD_ROW][dashPos], "%s", displayRowBuffer);
        dashPos += strlen(displayRowBuffer);
    }

    // update the full display
    updateDisplay();
}

void ControlLayer::subChannelHopped () {
    updateDashboard();
}

void ControlLayer::pingReceived (uint8_t deviceAddress) {
    Logger::info("Ping received", LogAppControl);
}

void ControlLayer::yieldForProcessing () {
    // sleep
    //vTaskDelay(10);
    //lv_timer_handler();
}

void ControlLayer::rotateDisplay () {
    if (millis() - titleLastRotation > TITLE_ROTATION_FREQUENCY) {
        switch (currTitleItem) {
            case TitleAlias:
                currTitleItem = TitleTime;
                sprintf(titleLine, control->getChatter()->getRtc()->getViewableTime());
                break;
            case TitleTime:
                currTitleItem = TitlePosition;
                double lat;
                double lng;
                if(control->getGpsCoords(lat, lng)) {
                    sprintf(titleLine, "%.6f, %.6f", lat, lng);
                }
                else {
                    sprintf(titleLine, "%s", "Location: [unavailable]");
                }
                break;
            default:
                currTitleItem = TitleAlias;
                sprintf(titleLine, "%s @ %s", control->getChatter()->getDeviceAlias(), control->getChatter()->getClusterAlias());
                break;
        }
        updateTitle(titleLine);
        titleLastRotation = millis();
    }
}

void ControlLayer::updateDisplay () {
    updateDisplay(displayLines);
}

void ControlLayer::updateDisplay (const char* dispText, uint8_t line) {
    if (line < DISPLAY_NUM_LINES && strlen(dispText) < DISPLAY_LINE_WIDTH) {
        sprintf(&displayLines[line][0], "%s", dispText);
        updateDisplay();
    }
    else {
        Logger::warn("Display request is too large for buffer: ", dispText, LogUi);
    }
}

void ControlLayer::updateTitle (const char* title) {
    updateDisplay(title, DISPLAY_TITLE_ROW);
}

void ControlLayer::updateSubtitle (const char* subtitle) {
    updateDisplay(subtitle, DISPLAY_DASHBOARD_ROW);
}


void ControlLayer::updateDisplay (const char lines[DISPLAY_NUM_LINES][DISPLAY_LINE_WIDTH]) {
    if (displayRunning) {
        bool somethingChanged = false;

        // clear any lines that were changed
        for (uint8_t line = 0; line < DISPLAY_NUM_LINES; line++) {
            bool changed = false;
            for (uint8_t charCount = 0; charCount < DISPLAY_LINE_WIDTH; charCount++) {
                if (lines[line][charCount] != lastDisplayLines[line][charCount]) {
                    lastDisplayLines[line][charCount] = lines[line][charCount];
                    changed = true;
                }
            }
            if (changed) {
                somethingChanged = true;

                display->setColor(BLACK);
                display->fillRect(1, line * DISPLAY_LINE_HEIGHT + (.5 * DISPLAY_LINE_HEIGHT), display->width() - 2, DISPLAY_LINE_HEIGHT);
                display->display();
            }
        }

        if (lastGeneralProgress != generalProgress) {
            display->setColor(BLACK);
            display->fillRect(0,0,1,display->height());
            display->display();
            somethingChanged = true;
        }

        if (lastMeshCachePct != meshCachePct) {
            display->setColor(BLACK);
            display->fillRect(display->width() - 1,0,1,display->height());
            display->display();
            somethingChanged = true;
        }


        if (somethingChanged) {
            display->setColor(WHITE); //?

            // draw general progress line on the right
            uint8_t lineHeight = generalProgress * display->height();
            display->drawLine(
                1, display->height() - lineHeight,
                1, display->width()
            );

            for (uint8_t l = 0; l < DISPLAY_NUM_LINES; l++) {
                // starting x position - screen width minus string width  / 2
                int x0 = (display->width() - display->getStringWidth(lines[l], strlen(lines[l]))) / 2;

                // starting y position - screen height minus string height / 2
                int y0 = (((display->height() - DISPLAY_LINE_HEIGHT)) / 2) + (((l - 1) * (DISPLAY_LINE_HEIGHT) + 4));

                // Draw the text - color of black (0)
                display->drawString(x0, y0, lines[l]);
            }

            // draw mesh cache line on the right
            lineHeight = meshCachePct * display->getHeight();
            display->drawLine(
                display->width() - 1, display->height() - lineHeight,
                display->width() - 1, display->height()
            );
            display->display();
        }
    }
}

const char* ControlLayer::getStatusName (ChatStatus chatStatus) {
    //logConsole("finding status for: " + String(chatStatus));
    switch (chatStatus) {
      case ChatDisconnected:
        return "X";
      case ChatConnecting:
        return "|";
      case ChatConnected:
        return " ";
      case ChatReceiving:
        return "<";
      case ChatReceived:
        return "<<";
      case ChatSending:
        return ">";
      case ChatSent:
        return ">>";
      case ChatFailed:
        return "!";
      case ChatNoDevice:
        return "X";
    }
    return "?";
  }