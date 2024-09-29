#include "ControlMode.h"

ControlMode::ControlMode (DeviceType _deviceType, RTClockBase* _rtc, CallbackRegistry* _callbackRegistry, uint8_t _sdPin, SPIClass & _sdSpiClass, XPowersLibInterface* _pmu) { 
  deviceType = _deviceType; 
  rtc = _rtc; 
  sdPin = _sdPin, 
  globalCallbackRegistry = _callbackRegistry;
  sdSpiClass = &_sdSpiClass;
  pmu = _pmu;
}

StartupState ControlMode::initEncryptedStorage () {
  controlModeInitializing = true;
  Logger::info("Encryption Level: ", STRONG_ENCRYPTION_ENABLED ? "STRONG" : "exportable", LogAppControl);

  if(rtc->isFunctioning()) {
    Logger::info("RTC Time: ", rtc->getViewableTime(), LogAppControl);

    #if defined(STORAGE_FRAM_SPI)
      chatter = new Chatter(ChatterDeviceBase, BasicMode, rtc, StorageFramSPI, this, this, this, this, STRONG_ENCRYPTION_ENABLED);
    #elif defined(STORAGE_SD_CARD)
      chatter = new Chatter(ChatterDeviceBase, BasicMode, rtc, StorageSD, this, this, this, this, STRONG_ENCRYPTION_ENABLED);
    #endif

    preferenceHandler = new PreferenceHandlerImpl(chatter, this);

    if (chatter->initEncryptedStorage()) {
      return StartupEncryptedStorageReady;
    }
  }
  return StartupError;
}

StartupState ControlMode::initLicense() {
  if (!chatter->initLicense()) {
    return StartupUnlicensed; // prompt user for license key
  }
  return StartupLicensed; // license is ok
}

StartupState ControlMode::initDeviceStore () {
  if(chatter->initDeviceStore()) {
    if (chatter->hasDevicePassword()) {
      Logger::info("Password is required", LogAppControl);

      return StartupNeedPassword; // prompt user for password
    }

    Logger::info("Password is NOT required", LogAppControl);
    return StartupDeviceStoreReady; // no password, storage is ready
    
  }
  return StartupError; // device store should never fail to init
}

bool ControlMode::hasDevicePassword() {
  return chatter->hasDevicePassword();
}

bool ControlMode::deviceLogin (const char* userPassword, uint8_t passwordLength) {
  return chatter->deviceLogin(userPassword, passwordLength);
}

StartupState ControlMode::finishDeviceInit () {
  if (chatter->finishDeviceInit()) {
    return StartupDeviceInitialized;
  }
  return StartupInitializeDevice; // need to initialize device
}

StartupState ControlMode::startChatter() {
  if (chatter->unlockStorage()) {
    #ifdef CHATTER_LORA_ENABLED
      if (preferenceHandler->isPreferenceEnabled(PreferenceLoraEnabled)) {
        showStatus("Init LoRa...");
        chatter->addLoRaChannel(LORA_CS, LORA_INT, LORA_RS, LORA_BUSY);
      }
      else {
        Logger::debug("Device LoRa disabled by user pref", LogAppControl);
      }

    #endif

    #ifdef CHATTER_WIFI_ENABLED
      if (preferenceHandler->isPreferenceEnabled(PreferenceWifiEnabled)) {
        #if defined(CHATTER_ONBOARD_WIFI)
          chatter->addOnboardUdpChannel (UDP_CHANNEL_LOG_ENABLED);
        # else
          chatter->addAirliftUdpChannel (SPIWIFI_SS, SPIWIFI_ACK, ESP32_RESETN, ESP32_GPIO0);
        #endif
      }
      else {
        Logger::debug("Device WiFi disabled by user pref", LogAppControl);
      }
    #endif

    #ifdef CHATTER_UART_ENABLED
      if (preferenceHandler->isPreferenceEnabled(PreferenceWiredEnabled)) {
        showStatus("Init UART...");
        chatter->addUartChannel();
      }
      else {
        Logger::debug("Device UART disabled by user pref", LogAppControl);
      }

    #endif

    chatter->setKeyForwardingAllowed(preferenceHandler->isPreferenceEnabled(PreferenceKeyForwarding));
    chatter->setTruststoreLocked(preferenceHandler->isPreferenceEnabled(PreferenceTruststoreLocked));
    chatter->setLocationSharingEnabled(preferenceHandler->isPreferenceEnabled(PreferenceLocationSharingEnabled));
    chatter->setLocationDeviceType(LocationDeviceHighPrecision);

    remoteConfigEnabled = preferenceHandler->isPreferenceEnabled(PreferenceRemoteConfigEnabled);
    if (remoteConfigEnabled) {
      Logger::info("Alert: Remote config is enabled!", LogAppControl);
    }

    // force pruning on startup. otherwise, repeated startups
    // can get really slow
    showStatus("Prune storage");
    chatter->pruneStorage(true);

    //chatter->getMeshPacketStore()->clearAllPackets();

    closeStorage();

    // set up the storage zone storage data
    for (uint8_t zoneCount = 0; zoneCount < CHATTER_STORAGE_ZONE_COUNT; zoneCount++) {
      nextFlush[zoneCount] = 0;

      switch (zoneCount) {
        case StorageZoneMessages:
          flushDelay[zoneCount] = 20000;
          break;
        case StorageZonePackets:
          flushDelay[zoneCount] = 120000;
          break;
        case StorageZoneMeshPackets:
          flushDelay[zoneCount] = 60000;
          break;
        case StorageZonePingTable:
          flushDelay[zoneCount] = 90000;
          break;
        case StorageZoneMeshGraph:
          flushDelay[zoneCount] = 60000*5;
          break;
        case StorageZoneLocations:
          flushDelay[zoneCount] = 70000;
          break;
        default:
          flushDelay[zoneCount] = 30000;
          break;
      }

    }

    controlModeInitializing = false;

    return StartupComplete;
  }

  Logger::error("Unlock storage failed!", LogAppControl);
  return StartupError;
}

void ControlMode::handleStartupError() {
  Logger::error("Startup error, no handler code!", LogAppControl);
}

void ControlMode::handleUnlicensedDevice () {
  Logger::error("No license!", LogAppControl);
  while(true) {
    delay(50);
  }
}

void ControlMode::clearMeshPackets () {
  meshPacketClearQueued = true;
}

bool ControlMode::clearMeshPacketsIfQueued () {
  if (meshPacketClearQueued) {
    openStorage();
    chatter->getMeshPacketStore()->clearAllPackets();
    closeStorage();
    meshPacketClearQueued = false;
    return true;
  }

  return false;
}

void ControlMode::clearMessages () {
  Logger::info("All messages will be cleared during next flush", LogAppControl);
  // wipe the message storage
  chatter->getMessageStore()->clearAllMessages();
}

// chat status callback
void ControlMode::subChannelHopped () {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->subChannelHopped();
}

void ControlMode::yieldForProcessing () {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->yieldForProcessing();
}


void ControlMode::updateChatStatus (uint8_t channelNum, ChatStatus newStatus) {

  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewStatus(channelNum, (ChatViewStatus)newStatus);
  //updateChatDashboard();
}

void ControlMode::updateChatStatus (const char* statusMessage) {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewStatus(statusMessage);
}

void ControlMode::updateChatProgress (float progress) {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewProgress(progress);
}

void ControlMode::resetChatProgress () {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewProgress(0.0);
}

void ControlMode::hideChatProgress () {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewProgress(0.0);
}

void ControlMode::showStartupScreen (float progress) {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewProgress(progress);
}

void ControlMode::showStatus (const char* status) {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateChatViewStatus(status);
}

void ControlMode::pingReceived (uint8_t deviceAddress) {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->pingReceived(deviceAddress);
}

void ControlMode::ackReceived(const char* sender, const char* messageId) {
  Logger::info("ack received", LogAppControl);
}

bool ControlMode::userInterrupted () {
  return ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->userInterrupted();
}

void ControlMode::updateMeshCacheUsed (float percent) {
  ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->updateCacheUsed(percent);
}

void ControlMode::resetBackupProgress() {
  Logger::info("reset backup progress", LogAppControl);
}

void ControlMode::updateBackupProgress (float pct) {
  Logger::debug("backup progress update", LogAppControl);
}

uint8_t ControlMode::promptBackupPassword (char* buffer) {
  Logger::error("prompt backup pw", LogAppControl);
  return 0;
}

void ControlMode::queueOutMessage (uint8_t* newMessage, int newMessageLength, const char* recipient, unsigned long scheduledDelay) {
  memcpy(outMessageBuffer, newMessage, newMessageLength);
  outMessageBufferLength = newMessageLength;
  sprintf(outMessageRecipient, "%s", recipient);
  outMessageBufferType = MessageTypePlain;
  outMessageStatus = ControlMessageNew;
  outMessageScheduledTime = millis() + scheduledDelay;
}

void ControlMode::queueOutBroadcast (uint8_t* newMessage, int newMessageLength, unsigned long scheduledDelay) {
  memcpy(outMessageBuffer, newMessage, newMessageLength);
  outMessageBufferLength = newMessageLength;
  sprintf(outMessageRecipient, "%s", chatter->getClusterBroadcastId());
  outMessageBufferType = MessageTypePlain;
  outMessageStatus = ControlMessageNew;
  outMessageScheduledTime = millis() + scheduledDelay;
}

void ControlMode::queuePasswordChange (const char* newPassword) {
  memset(passwordBuffer, 0, 17);
  sprintf(passwordBuffer, "%s", newPassword);
  changePasswordQueued = true;
}

bool ControlMode::changePassword () {
  bool result = chatter->getDeviceStore()->changePassphrase (passwordBuffer, strlen(passwordBuffer), chatter->getPasswordTestPhrase());
  memset(passwordBuffer, 0, 17); // clear password buffer
  if (result) {
    Logger::warn("Password successfully changed", LogAppControl);
  }
  else {
    Logger::error("Password change failed!", LogAppControl);
  }
  return result;
}

void ControlMode::cachePasswordHash (const char* password) {
  memset(passwordHash, 0, 33);

  // store the hash of the password for later (screen unlocks, etc)
  hasher.clear();
  hasher.update(password, strlen(password));
  hasher.finalize(passwordHash, hasher.hashSize());
  hasher.clear();

  hasPassword = true; // might not be the best place to do this
}

// validates the user password against previously stored
bool ControlMode::verifyPassword (const char* password) {
  memset(passwordTestHash, 0, 33);

  // store the hash of the password for later (screen unlocks, etc)
  hasher.clear();
  hasher.update(password, strlen(password));
  hasher.finalize(passwordTestHash, hasher.hashSize());
  hasher.clear();

  return memcmp(passwordHash, passwordTestHash, 32) == 0;
}

void ControlMode::notifyUiInterruptReceived () {
  if (chatter != nullptr) {
    chatter->interruptChatter ();
  }
}

void ControlMode::processOneCycle(ControlCycleType cycleType) {
  if (factoryResetQueued) {
    listeningForMessages = false;
    wipeStorage();
    closeStorage();
    restartDevice();

    while(true) {
      delay(10);
    }
  }  
  else if (changePasswordQueued) {
    changePasswordQueued = false;
    openStorage();
    changePassword();
    restartQueued = true; // flush storage and restart
  }

  if (restartQueued) {
    Logger::info("Flushing and restarting", LogAppControl);
    openStorage();
    flushStorage();
    closeStorage();
    restartDevice();

    while(true) {
      delay(10);
    }
  }
  else if (meshResetQueued) {
    meshResetQueued = false;
    Logger::warn("Resetting mesh", LogAppControl);
    openStorage();
    chatter->resetMesh();
    flushStorage();
    closeStorage();
  }

  int numPacketsThisCycle = 0;

  // reset the progress
  updateChatProgress(0.0);

  if (outMessageStatus == ControlMessageNew) {
    // if we've reached the scheduled time, send it
    if (millis() >= outMessageScheduledTime){
      outMessageStatus = ControlMessageScheduled;
    }
  }
  else if (outMessageStatus == ControlMessageScheduled) {
    outMessageScheduledTime = 0; // clear the schedule timer, since its getting picked up now
    outMessageStatus = ControlMessageSendingDirect;

    // is it broadcast or DM
    if (memcmp(outMessageRecipient, chatter->getClusterBroadcastId(), CHATTER_DEVICE_ID_SIZE) == 0) {
      if(chatter->broadcast(outMessageBuffer, outMessageBufferLength)) {
        outMessageStatus = ControlMessageSentDirect;
        Logger::info("Broadcast sent", LogAppControl);
      }
      else {
        outMessageStatus = ControlMessageFailed;
        Logger::warn("Broadcast sent", LogAppControl);
      }
    }
    else {
      // try sending direct
      ChatterMessageFlags flags;
      flags.Flag0 = outMessageBufferType;
      flags.Flag2 = AckRequestTrue;

      if(chatter->send(outMessageBuffer, outMessageBufferLength, outMessageRecipient, &flags)) {
        outMessageStatus = ControlMessageSentDirect;
      }
    }
  }
  else if (outMessageStatus == ControlMessageSendingDirect) {
      // new failed, try sending mesh
    ChatterMessageFlags flags;
    flags.Flag0 = messageBufferType;

    // drop message into mesh
    if (chatter->sendViaMesh(outMessageBuffer, outMessageBufferLength, outMessageRecipient, &flags)) {
      outMessageStatus = ControlMessageMeshQueued;
    }
    else {
      outMessageStatus = ControlMessageFailed;
    }
  }
  else if (listeningForMessages) {
    bool userInt = false;
    showStatus("Listening");
    while (chatter->hasMessage(cycleType == ControlCycleFull ? CHATTER_POLL_COUNT_PER_CYCLE_FULL : CHATTER_POLL_COUNT_PER_CYCLE_RESPONSIVE) && numPacketsThisCycle++ < maxReadPacketsPerCycle) {
      showStatus("Receiving");
      if(chatter->retrieveMessage() && chatter->getMessageType() == MessageTypeComplete) {
        Logger::info("Processing trusted message...", LogAppControl);

        messageBufferLength = chatter->getMessageSize();
        memcpy(messageBuffer, chatter->getTextMessage(), messageBufferLength);
        messageBuffer[messageBufferLength] = 0;
        memcpy(otherDeviceId, chatter->getLastSender(), CHATTER_DEVICE_ID_SIZE);
        otherDeviceId[CHATTER_DEVICE_ID_SIZE] = 0;

        // send ack (later, queue this)
        if (!chatter->isAcknowledgement()) {
          if (memcmp(chatter->getLastRecipient(), chatter->getDeviceId(), CHATTER_DEVICE_ID_SIZE) == 0) {
            Logger::debug("sending ack..", LogAppControl);
            if(!chatter->sendAck(otherDeviceId, chatter->getMessageId())) {
              Logger::debug("Ack direct failed", LogAppControl);
              if (chatter->isMeshEnabled()) {
                chatter->sendAckViaMesh(otherDeviceId, chatter->getMessageId());
              }
            }
          }
        }

        // if it's a command, execute it
        if (chatter->getMessageFlags().Flag0 == MessageTypeControl && isRemoteCommand(messageBuffer, messageBufferLength)) {
          // in node, remote commands are always enabled
          //if (preferenceHandler->isPreferenceEnabled(PreferenceRemoteConfigEnabled)) {
            Logger::warn("Executing command: ", (const char*)messageBuffer, LogAppControl);
            executeRemoteCommand(messageBuffer, otherDeviceId);
          //}
          //else {
          //  Logger::warn("Received remote command, but not enabled on this device!", LogAppControl);
          //}
        }
        else {
          ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->messageReceived();
        }

      }

      ((ChatterViewCallback*)globalCallbackRegistry->getCallback(CallbackChatStatus))->yieldForProcessing();
      userInt = userInterrupted();
    }

    // sync every loop, strategy decides how often
    if (numPacketsThisCycle == 0 && userInt == false) {
      if (clearMeshPacketsIfQueued() == false) {
        showStatus("Mesh");
        if(chatter->syncMesh()) {
          Logger::info("Mesh activity occurred", LogAppControl);
        }
      }
    }
    showStatus("Ready");

    if (userInterrupted()) {
      return;
    }

    // gps if time
    /*if (numPacketsThisCycle == 0) {
      refreshGpsCoords();
    }*/

    // the timings of these are controlled within chatter layer
    if (cycleType == ControlCycleFull && chatter->isTimeToPruneStorage()) {
      if (openStorage()) {
        chatter->pruneStorage();
        closeStorage();
      }
      else {
          Logger::warn("Storage unavailable for pruning", LogAppControl);
      }
    }
  }
}

// does an immediate factory reset
void ControlMode::factoryReset () {
  Logger::warn("Factory resetting!", LogAppControl);
  factoryResetQueued = true;  
}

bool ControlMode::flushStorage () {
  bool flushHappened = false;
  unsigned long now = millis();

  // check each zone for whether flush is needed
  // stop after any single flush, so we dont trigger timeout
  for (uint8_t zone = 0; zone < CHATTER_STORAGE_ZONE_COUNT; zone++) {
    if (nextFlush[zone] != 0 && now > nextFlush[zone]) {
      if (chatter->isStorageDirty((StorageZone)zone)) {
        char logBuff[32];
        sprintf(logBuff, "flushing zone %d", zone);
        Logger::info(logBuff, LogAppControl);
        if (openStorage()) {
          chatter->flushStorage((StorageZone)zone);
          closeStorage();
          nextFlush[zone] = 0;

          flushHappened = true;
          zone = CHATTER_STORAGE_ZONE_COUNT;
        }
        else {
          Logger::warn("Storage unavailable for flush", LogAppControl);
        }
      }
    }
  }

  if (flushHappened == false) {
    // check each zone to see if another flush should be scheduled
    for (uint8_t zone = 0; zone < CHATTER_STORAGE_ZONE_COUNT; zone++) {
      if (nextFlush[zone] == 0 && chatter->isStorageDirty((StorageZone)zone)) {
        if (chatter->isStorageDirty((StorageZone)zone)) {
          nextFlush[zone] = now + flushDelay[zone];
        }
      }
    }
  }

  return flushHappened;
}

void ControlMode::showTime () {
  //rtc->populateTimeDateString(timeDate, true);
  Logger::debug(rtc->getViewableTime(), LogAppControl);
}

/*****************
 * Miscellaneous *
 *****************/
void ControlMode::sleepOrBackground(unsigned long sleepTime) {
  delay(sleepTime);
}

bool ControlMode::initializeNewDevice (DeviceInitializationForm* initializationForm) {
  Logger::warn("ControlMode Initializing New Device", LogAppControl);
  // by default, we just generate random stuff
  // if interactive, a subclass will handle this, and prompt for info
  ClusterAdmin* admin = new ClusterAdmin(chatter, STRONG_ENCRYPTION_ENABLED);

  ClusterNumberFrequencies cnf = ClusterFreq64;
  int numClusterChannels = atoi(initializationForm->numChannels);
  switch (numClusterChannels) {
    case 1:
      cnf = ClusterFreq1;
      break;
    case 8:
      cnf = ClusterFreq8;
      break;
    case 16:
      cnf = ClusterFreq16;
      break;
    case 32:
      cnf = ClusterFreq32;
      break;
  }

  ClusterFrequencyHopping cfh = ClusterFrequencyHopNever;
  if (numClusterChannels > 1) {
    const char* selectedHopSchedule = initializationForm->hopSchedule;
    if (strcmp(selectedHopSchedule, "10") == 0) {
      cfh = ClusterFrequencyHopTenSec;
    }
    else if (strcmp(selectedHopSchedule, "100") == 0) {
      cfh = ClusterFrequencyHopHundredSec;
    }
    else if (strcmp(selectedHopSchedule, "1000") == 0) {
      cfh = ClusterFrequencyHopThousandSec;
    }
  }

  bool success = admin->genesis (
    initializationForm->deviceAlias, 
    initializationForm->devicePassword,
    strlen(initializationForm->devicePassword),
    initializationForm->clusterAlias, 
    atof(initializationForm->centerFrequency),
    false, 
    "", 
    "",
    false,
    cfh,
    cnf
    );

  if (success) {
    // store DST and time zone preferences
    chatter->getDeviceStore()->setDstEnabled(false);
    chatter->getDeviceStore()->setTimeZone(getTimeZoneFor(initializationForm->timeZone));
    Logger::warn("ControlMode Initialization complete", LogAppControl);
  }
  else {
    Logger::error("ControlMode Initialization FAILED", LogAppControl);
  }

  return success;
}

bool ControlMode::getGpsCoords (double& latitude, double& longitude) {
  showStatus("GPS");
  if (rtc->getGpsIsValid()) {
    latitude = rtc->getLatitude();
    longitude = rtc->getLongitude();

    chatter->getLocationStore()->updateLocation(chatter->getDeviceId(), rtc->getEpoch(), latitude, longitude, LocationDeviceMediumPrecision);

    return true;
  }

  return false;
}

void ControlMode::restartDevice() {
  Logger::warn("Restarting", LogAppControl);
  delay(100);
  ESP.restart();
}

uint8_t ControlMode::promptLicenseKey (char* buffer) {
  Logger::debug("prompt for license key...", LogAppControl);
  return 16;
}

void ControlMode::licenseValidation (bool isValid) {
  if (isValid) {
    Logger::debug("License IS valid", LogAppControl);
  }
  else {
    Logger::debug("License is NOT valid", LogAppControl);
  }
}

TimeZoneValue ControlMode::getTimeZoneFor (const char* tzName) {
  const char* zoneNames[25] = {
    "London",
    "Paris/Berlin",
    "Cairo",
    "Jeddah",
    "Dubai",
    "Karachi",
    "Dhaka",
    "Bankok",
    "Beijing",
    "Tokyo",
    "Sydney",
    "Noumea",
    "Wellington",
    "Tongatapu",
    "Midway Island",
    "Honolulu",
    "Anchorage",
    "Los Angeles",
    "Denver",
    "Chicago",
    "New York",
    "Santa Domingo",
    "Rio de Janeiro",
    "Fernando de Noronha",
    "Azores Islands"    
  };

  for (int z = 0; z < 25; z++) {
    if (memcmp(tzName, zoneNames[z], strlen(zoneNames[z])) == 0) {
      return (TimeZoneValue)z;
    }
  }

  // deafult to ny
  return TZ_NY;
}

bool ControlMode::factoryResetButtonHeld() {
  #ifdef FACTORY_RESET_PIN
  // check if factory rest button being held
  if (!digitalRead(FACTORY_RESET_PIN)) {
    showStatus("Factory Reset...");

    // pin needs to be held down for 3 seconds
    int resetMillis = 10000;
    bool held = true;
    for (uint8_t timerCount = 0; held && timerCount < resetMillis / 250; timerCount++) {
      Logger::warn("Button held, factory reset countdown", LogAppControl);
      held = !digitalRead(FACTORY_RESET_PIN);
      delay(250);
    }
    if (held) {
      Logger::warn("Factory reset by button upon startup", LogAppControl);
      return true;
    }
  }
  #endif

  return false;

}

// close sd card resources , if they are currently open
bool ControlMode::closeStorage () {
  /*#if !defined(CORE_TEENSY) // END not supported on teensy?!?
  if (storageOpen == true) {
    Logger::info("Closing storage", LogAppControl);
    SD.end(); 
    Logger::info("Storage is closed", LogAppControl);
    storageOpen = false;
  }
  #endif
  */
  return true;
}

bool ControlMode::wipeStorage () {
  openStorage();
  char fullFileName[64];
  char fullFolderName[64];

  // delete every file before the current
  char* rootFolder = "/fram/chatter";

  for (uint8_t folderNum = 0; folderNum < 12; folderNum++) {
    sprintf(fullFolderName, "%s/%d", rootFolder, folderNum);    
    File subFolder =  SD.open(fullFolderName);
    if (subFolder) {
      Logger::warn("Clearing folder: ", fullFolderName, LogAppControl);
      while (true) {
        File datafile = subFolder.openNextFile();
        if (! datafile) {
            break;
        }
        const char* dataFileName = datafile.name();
        datafile.close();

        Logger::debug("Removing file: ", dataFileName, LogAppControl);

        sprintf(fullFileName, "%s/%s", fullFolderName, dataFileName);
        if (SD.remove(fullFileName)) {
          Logger::debug("Removed data file: ", fullFileName, LogAppControl);
        }
        else {
          Logger::error("Failed to remove: ", fullFileName, LogAppControl);
        }
      }
      subFolder.close();
      if (SD.rmdir(fullFolderName)) {
        Logger::debug("Removed directory: ", fullFolderName, LogAppControl);
      }
      else {
        Logger::warn("Failed to remove directory: ", fullFolderName, LogAppControl);
      }
    }
  }
  SD.rmdir(rootFolder);

  closeStorage();
  return true;
}

// open sd card resources. Does nothing if already open
bool ControlMode::openStorage ()  {
  if (!storageOpen) {
    Logger::info("Opening storage", LogAppControl);
    if (SD.begin(sdPin, *sdSpiClass)) {
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Logger::warn("No SD card attached", LogStorage);
            return false;
        } 
        else {
            Logger::info("Storage is open", LogAppControl);
            storageOpen = true;
        }
    }
    else {
        Logger::error("Open storage failed!", LogAppControl);
    }
  }

  return storageOpen;
}


void ControlMode::joinCluster () {
  while (true) {
    continueJoining();
  }
}

void ControlMode::continueJoining () {
    if (joiningInitialized == false) {
        if (assistant == nullptr) {
            assistant = new ChatterClusterAssistant(chatter, LORA_CS, LORA_INT, LORA_RS, LORA_BUSY, false, STRONG_ENCRYPTION_ENABLED);
        }
        if(assistant->init()) {
            assistant->beginOnboarding();
            joiningInitialized = true;
            Logger::info("Joining initialized", LogUi);
        }
    }
    else {
        // step forward in onbaord
        assistant->onboardNextStep();
        bool isConnected = assistant->isConnected();
        bool isComplete = assistant->isOnboardComplete();
        bool isTrying = assistant->isOnboardingInProgress();

        if (isConnected == false) {
          updateChatStatus("Waiting for Connect");
        }
        else if (isComplete) {
            // since joining a new cluster, clear the mesh/ping info to make
            // room for the new cluster's data
            chatter->getDeviceStore()->setClearMeshOnStartup(true);

            // save the changes
            flushStorage();

            delete assistant;
            assistant = nullptr;

            // restart to join new cluster
            Logger::info("Join is complete, restarting...", LogUi);
            updateChatStatus("Joined; Restarting....");
            delay(5000);
            restartDevice();
        }
    }
}


bool ControlMode::isRemoteCommand (const uint8_t* msg, int msgLength) {
  return msgLength >= 5 && memcmp(REMOTE_COMMAND_PREFIX, msg, 3) == 0 && msg[3] == ':';
}

bool ControlMode::isBackpackRequest (const uint8_t* msg, int msgLength) {
  return msgLength >= 4 && memcmp("BK:", msg, 3) == 0;
}

float ControlMode::getBatteryLevel() {
  if (pmu->isBatteryConnect()) {
    return pmu->getBatteryPercent();
  }
  return 1.0;
}

bool ControlMode::executeRemoteCommand (uint8_t* message, const char* requestor) {
  switch (message[4]) {
    case RemoteCommandBattery:
      // grab the battery level
      sprintf((char*)messageBuffer, "%s %03d", "Battery:", getBatteryLevel());
      Logger::info("RC Sending battery level to: ", requestor, LogAppControl);

      // send to requestor
      queueOutMessage(messageBuffer, strlen((char*)messageBuffer), requestor, 500);
      return true;
    case RemoteCommandUptime:
      sprintf((char*)messageBuffer, "Uptime: %d min", (millis() / 1000)/60);
      Logger::info("RC Sending uptime to: ", requestor, LogAppControl);

      // send to requestor
      queueOutMessage(messageBuffer, strlen((char*)messageBuffer), requestor, 500);
      return true;
    case RemoteCommandNeighbors:
      rcNeighborCount = chatter->getPingTable()->loadNearbyDevices (PingQualityBad, rcNeighbors, 10, 90);
      memset(messageBuffer, 0, GUI_MESSAGE_BUFFER_SIZE);

      char* pos = (char*)messageBuffer;
      const char* neighborsPrefix = "Neighbors: ";
      memcpy(pos, neighborsPrefix, strlen(neighborsPrefix));
      pos += strlen(neighborsPrefix);

      for (uint8_t i = 0; i < rcNeighborCount; i++) {
        if (i > 0){
          memcpy(pos, ", ", 2);
          pos += 2;
        }

        // copy either the device id or alias
        memset(meshDevIdBuffer, 0, CHATTER_DEVICE_ID_SIZE + 1);
        memset(meshAliasBuffer, 0, CHATTER_ALIAS_NAME_SIZE + 1);
        chatter->loadDeviceId(rcNeighbors[i], meshDevIdBuffer);
        if (chatter->getTrustStore()->loadAlias(meshDevIdBuffer, meshAliasBuffer)) {
          // copy the alias
          memcpy(pos, meshAliasBuffer, strlen(meshAliasBuffer));
          pos += strlen(meshAliasBuffer);
        }
        else {
          // just copy device id
          memcpy(pos, meshDevIdBuffer, CHATTER_DEVICE_ID_SIZE);
          pos += CHATTER_DEVICE_ID_SIZE;
        }
      }

      Logger::info("RC neighbors to: ", requestor, LogAppControl);

      // send to requestor
      queueOutMessage(messageBuffer, strlen((char*)messageBuffer), requestor, 500);
      return true;
  }

  Logger::warn("unknown remote config", LogAppControl);
  return false;
}

void ControlMode::populateMeshPath (const char* recipientId) {
  meshPathLength = chatter->findMeshPath (chatter->getDeviceId(), recipientId, meshPath);

  // copy the path into the message buffer so we can display
  memset(messageBuffer, 0, GUI_MAX_MESSAGE_LENGTH+1);
  uint8_t* pos = messageBuffer;

  if (meshPathLength > 0) {
    for (uint8_t p = 0; p < meshPathLength; p++) {
      memset(meshDevIdBuffer, 0, CHATTER_DEVICE_ID_SIZE + 1);
      memset(meshAliasBuffer, 0, CHATTER_ALIAS_NAME_SIZE + 1);

      // find cluster device with that address
      chatter->loadDeviceId(meshPath[p], meshDevIdBuffer);

      // laod from truststore
      if (chatter->getTrustStore()->loadAlias(meshDevIdBuffer, meshAliasBuffer)) {
        memcpy(pos, meshAliasBuffer, strlen(meshAliasBuffer));
        pos += strlen(meshAliasBuffer);
      }
      else {
        sprintf((char*)pos, "%c%s%c", '[', meshDevIdBuffer, ']');
        pos += (2 + CHATTER_DEVICE_ID_SIZE);
      }

      if (p+1 < meshPathLength) {
        memcpy(pos, " -> ", 4);
        pos += 4;
      }
    }
  }
  else {
    memcpy(pos, "no path!", 8);
  }  
}
