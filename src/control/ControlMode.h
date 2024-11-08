
#include <Arduino.h>
#include <stdint.h>
#include "../globals/Globals.h"
#include "ChatterAll.h"
#include "../events/CommunicatorEvent.h"
#include "../callbacks/CallbackRegistry.h"
#include "../callbacks/ChatterViewCallback.h"
#include "../events/RemoteCommand.h"
#include "BasicControl.h"
#include "../prefs/PreferenceHandler.h"
#include "../prefs/PreferenceHandlerImpl.h"

#include "../forms/DeviceInitializationForm.h"
#include <SHA256.h>
#include <XPowersLib.h>
#include "../backpacks/relay/RelayBackpack.h"
#include "../backpacks/Backpack.h"

#ifndef CONTROL_MODE_H
#define CONTROL_MODE_H

enum EncodingType {
  EncodingTypeText = 1,
  EncodingTypeBinary = 2
};

enum BridgeRequestType {
  BridgeRequestNone = 1, // ignore
  BridgeRequestEcho = 2, // request message be echoed
  BridgeRequestBridge = 3, // request a bridge to other communication type
  BridgeRequestTime = 4, // request current time
  BridgeRequestIdentity = 5
};

enum AckRequestType {
  AckRequestTrue = 1, // ignore
  AckRequestFalse = 2
};

enum DeviceType {
  DeviceTypeCommunicator = 0,
  DeviceTypeCommunicatorMini = 1,
  DeviceTypeBase = 2
};

enum StartupState {
  StartupComplete = 1,
  StartupInitializeDevice = 2,
  StartupDeviceInitialized = 3,
  StartupError = 4,
  StartupUnlicensed = 5,
  StartupLicensed = 6,
  StartupNeedPassword = 7,
  StartupEncryptedStorageReady = 8,
  StartupDeviceStoreReady = 9
};

enum ControlMessageState {
  ControlMessageNew = 0,
  ControlMessageScheduled = 1,
  ControlMessageSendingDirect = 2,
  ControlMessageSendingMesh = 3,
  ControlMessageSentDirect = 4,
  ControlMessageMeshQueued = 5,
  ControlMessageFailed = 6,
  ControlMessageCancelled = 7,
  ControlMessageUnknown = 8
};

enum ControlCycleType {
  ControlCycleResponsive = 0,
  ControlCycleFull = 1
};

#define STORAGE_PRUNE_DELAY 60000*10 // 10 min
#define CHATTER_POLL_COUNT_PER_CYCLE_RESPONSIVE 2
#define CHATTER_POLL_COUNT_PER_CYCLE_FULL 10

/**
 * Base class for the different control modes available for this vehicle.
 */
class ControlMode : public ChatStatusCallback, public BackupCallback, public LicenseCallback, public StorageStatusCallback, public BasicControl {
  public:
    ControlMode (DeviceType _deviceType, RTClockBase* _rtc, CallbackRegistry* _callbackRegistry, uint8_t _sdPin, SPIClass & _sdSpiClass, XPowersLibInterface* _pmu);

    /** initialization methods **/
    virtual StartupState initEncryptedStorage();
    virtual StartupState initLicense();
    virtual StartupState initDeviceStore();
    virtual bool hasDevicePassword();
    virtual bool deviceLogin(const char* userPassword, uint8_t passwordLength);
    virtual StartupState finishDeviceInit();
    virtual StartupState startChatter();
    /** end init methods **/

    virtual void processOneCycle(ControlCycleType cycleType);

    virtual void showStatus (const char* status);

    virtual void showTime();
    virtual bool isInteractive () { return false; }

    String getFormattedDate ();
    String getFormattedTime ();

    // callback from chat status
    void updateChatStatus (uint8_t channelNum, ChatStatus newStatus);
    void updateChatStatus (const char* statusMessage);
    //void updateChatDashboard (); // redisplay chat dashboard
    void updateChatProgress(float progress);
    void resetChatProgress ();
    void hideChatProgress ();
    void pingReceived (uint8_t deviceAddress);
    void ackReceived(const char* sender, const char* messageId);
    bool userInterrupted ();
    void showStartupScreen (float progress);
    void subChannelHopped ();
    void yieldForProcessing ();
    void updateMeshCacheUsed (float percent);
    void setUiEnabled (bool enableInterrupt){}

    void notifyUiInterruptReceived ();


    float getChatProgress () { return chatProgress; }
    virtual void showBusy (const char* busyTitle, const char* busyDescription, const char* status, bool cancellable) = 0;
    virtual void updateBusyStatus (const char* status) = 0;

    // this should not bubble up to this layer
    bool shouldAcknowledge (uint8_t address, uint8_t* message, uint8_t length) { return false; }
    
    // touch screen, etc
    virtual bool isFullyInteractive () { return false; }
    virtual bool initializeNewDevice (DeviceInitializationForm* initializationForm);

    virtual void handleStartupError ();
    virtual void handleUnlicensedDevice ();

    void resetBackupProgress ();
    void updateBackupProgress (float pct);
    uint8_t promptBackupPassword (char* buffer);

    uint8_t promptLicenseKey (char* buffer);
    void licenseValidation (bool isValid);

    Chatter* getChatter () { return chatter; }
    
    /** basic control **/
    void restartDevice();
    void setLearningEnabled (bool _enabled) { learningModeEnabled = _enabled; }
    void setRemoteConfigEnabled (bool _enabled) { remoteConfigEnabled = _enabled; }
    float getBatteryLevel ();

    /*******************/

    void queueRestart() { restartQueued = true; }
    void queueMeshReset () { meshResetQueued = true; }
    void queuePasswordChange (const char* newPassword);
    void cachePasswordHash (const char* password);
    bool deviceHasPassword () { return hasPassword; }
    bool verifyPassword (const char* password);

    void setListeningForMessages(bool _listening) { listeningForMessages = _listening; }
    bool getGpsCoords (double& latitude, double& longitude);
    //bool refreshGpsCoords ();

    const char* getViewableTime() {return rtc->getViewableTime();}

    ControlMessageState getOutMessageStatus () { return outMessageStatus; }
    void cancelOutMessage () { outMessageStatus = ControlMessageCancelled; }
    void queueOutMessage (uint8_t* newMessage, int newMessageLength, const char* recipient, unsigned long scheduledDelay);
    void queueOutBroadcast (uint8_t* newMessage, int newMessageLength, unsigned long scheduledDelay);

    // queues a packet clearing
    void clearMeshPackets ();
    void clearMessages ();

    bool flushStorage (); // flushes if it's time
    bool closeStorage (); // close sd card resources
    bool openStorage (); // open sd card resources. Does nothing if already open
    bool wipeStorage ();

    // does an immediate factory reset
    void factoryReset ();
    void joinCluster();

    PreferenceHandler* getPreferenceHandler () { return preferenceHandler; }

  protected:
    bool executeRemoteCommand (uint8_t* message, const char* requestor);
    void populateMeshPath (const char* recipientId);
    bool isRemoteCommand (const uint8_t* msg, int msgLength);
    bool isBackpackRequest (const uint8_t* msg, int msgLength);
    RemoteCommandType getRemoteCommandFor (const char* commandName);

    bool clearMeshPacketsIfQueued ();
    bool meshPacketClearQueued = false;

    bool storageOpen = false; // storage will be opened during initialization automatically
    uint8_t sdPin = 0;
    SPIClass* sdSpiClass;
    
    /** fields for handling messages **/
    uint8_t messageBuffer[GUI_MESSAGE_BUFFER_SIZE+1];
    int messageBufferLength = 0;
    MessageType messageBufferType;
    char otherDeviceId[CHATTER_DEVICE_ID_SIZE+1];
    char otherClusterId[CHATTER_LOCAL_NET_ID_SIZE+CHATTER_GLOBAL_NET_ID_SIZE+1];

    uint8_t outMessageBuffer[GUI_MESSAGE_BUFFER_SIZE + 1];
    char outMessageRecipient[CHATTER_DEVICE_ID_SIZE+1];
    int outMessageBufferLength = 0;
    ControlMessageState outMessageStatus = ControlMessageUnknown;
    MessageType outMessageBufferType;
    unsigned long outMessageScheduledTime = 0;
    /****/

    /** password stuff */
    SHA256 hasher;
    bool hasPassword = false;
    char passwordTestHash[33];
    char passwordHash[33];
    char passwordBuffer[17];
    bool changePasswordQueued = false;
    bool changePassword ();
    /****/

    bool factoryResetButtonHeld ();

    // miscellaneous
    virtual void sleepOrBackground(unsigned long sleepTime);
    TimeZoneValue getTimeZoneFor (const char* tzName);
    bool listeningForMessages = false;
    bool controlModeInitializing = false;
    uint8_t maxReadPacketsPerCycle = 5;

    RTClockBase* rtc;
    Chatter* chatter;
    CallbackRegistry* globalCallbackRegistry;
    PreferenceHandler* preferenceHandler;

    ClusterAdminInterface* cluster = nullptr;
    ClusterAssistantInterface* assistant = nullptr;
    bool justOnboarded = false;

    long loopCount = 0;
    float chatProgress = 0;

    DeviceType deviceType;
    
    unsigned long nextGpsRrefresh = 0;
    unsigned long gpsRefreshDelay = 10000; // how often to refresh gps

    unsigned long nextFlush [CHATTER_STORAGE_ZONE_COUNT];
    unsigned long flushDelay [CHATTER_STORAGE_ZONE_COUNT];

    bool restartQueued = false;
    bool factoryResetQueued = false;

    bool remoteConfigEnabled = false;
    bool learningModeEnabled = false;

    bool meshResetQueued = false;

    void continueJoining ();
    bool joiningInitialized = false;

    uint8_t meshPath[CHATTER_MESH_MAX_HOPS];
    uint8_t meshPathLength = 0;
    char meshDevIdBuffer[CHATTER_DEVICE_ID_SIZE + 1];
    char meshAliasBuffer[CHATTER_ALIAS_NAME_SIZE + 1];        
    uint8_t rcNeighborCount = 0;
    uint8_t rcNeighbors[10];    
    XPowersLibInterface* pmu;

    char logBuffer[128];

    Backpack* backpacks[MAX_BACKPACKS];
    uint8_t numBackpacks = 0;
    Backpack* getBackpack (BackpackType type);
    Backpack* getBackpack (uint8_t* remoteRequest, int requestLength);

};
#endif