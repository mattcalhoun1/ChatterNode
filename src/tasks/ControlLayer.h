#include <Arduino.h>
#include "../control/ControlMode.h"
#include "../control/HeadlessControlMode.h"
#include "../events/ChatterUserEvent.h"
#include "../callbacks/CallbackRegistry.h"
#include "../callbacks/ChatterViewCallback.h"
#include "../forms/DeviceInitializationForm.h"
#include "../forms/NewClusterForm.h"
#include <SH1106Wire.h>

#ifndef CONTROLLAYER_H
#define CONTROLLAYER_H

#define DISPLAY_LINE_WIDTH 64
#define DISPLAY_NUM_LINES 3

#define DISPLAY_LINE_HEIGHT 16

#define DISPLAY_TITLE_ROW 0
#define DISPLAY_DASHBOARD_ROW 1
#define DISPLAY_STATUS_ROW 2

#define TITLE_ROTATION_FREQUENCY 5000 // rotate title every 5 sec

//#define MAX_USER_INPUT_LEN 1024
//#define MAX_SUBSCRIPTIONS_PER_VIEW 3 // no more than 3 views can be notified

enum TitleRotationItem {
    TitleAlias = 0,
    TitleTime = 1,
    TitlePosition = 2,
    TitleBattery = 3
};

enum ControlModeStatus {
  ControlModeIdle = 0,
  ControlModeStarting = 1,
  //ControlModeUnlicensed = 2,
  //ControlModeUninitialized = 3,
  ControlModeError = 4,
  ControlModeReady = 5,
  ControlModeReceiving = 6,
  ControlModeSending = 7,
  ControlModeMeshing = 8,
  ControlModeOther = 9,
  ControlModePaused = 10,
  ControlModeProcessing = 11, // processing method is not re-entrant

//  ControlStartupComplete = 12,
  ControlStartupInitializeDevice = 13,
  ControlStartupDeviceInitialized = 14,
  ControlStartupError = 15,
  ControlStartupUnlicensed = 16,
  ControlStartupLicensed = 17,
  ControlStartupNeedPassword = 18,
  ControlStartupReadyToUnlock = 19,
  ControlStartupEncryptedStorageReady = 20,
  ControlStartupDeviceStoreReady = 21
};

enum ControlMessagingStatus {
  MessagingRunning = 0,
  MessagingPaused = 1
};

#define MIN_REFRESH_INTERVAL 1000 // only one high priority refresh allowed during this interval
#define HIGH_PRIORITY_REFRESH_PAUSE 300 // how long to allow for the full refresh

#define USER_INTERACTION_PAUSE_TIMER 5000 // how long to pause messaging while the user interacts

class ControlLayer : public ChatterViewCallback {
  public:
    ControlLayer (CallbackRegistry* _callbackRegistry);
    ControlModeStatus getStatus () { return status; }
    void setControlMode (ControlMode* _control);

    ControlModeStatus initializeNextStep ();
    bool process (ChatterUserEvent evt);
    //bool processForCurrentView ();

    void setMessagingPaused (bool paused) { messagingStatus = paused ? MessagingPaused : MessagingRunning; }
    void setPaused (bool paused) { status = paused ? ControlModePaused : ControlModeReady; }
    bool isMessagingPaused () { return messagingStatus == MessagingPaused || (messagingPausedUntil != 0 && messagingPausedUntil > millis()); }
    void pauseMessagingFor(unsigned long pauseLengthMillis);
    //void setCurrentView (ChatterView view);
    //ChatterView getCurrentView () {return currentView;}
    //ChatterView getLastView() {return lastView;}

    //void userScrolled (UserScrollDirection scrollDirection);
    //void userClicked ();

    const char* getRawHardwareId();
    const char* getFormattedHardwareId();

    /** callbacks to receive from control **/
    void updateChatViewStatus (const char* status);
    void updateChatViewProgress (float progress);
    void updateCacheUsed (float pct);
    void updateChatViewStatus (uint8_t channelNum, ChatViewStatus newStatus);
    void subChannelHopped ();
    void pingReceived (uint8_t deviceAddress);
    bool userInterrupted () { return interruptedByUser; }
    void yieldForProcessing ();
    void messageReceived ();
    void userTyped(char typedChar);

    unsigned long getCurrentDelay ();
    //bool isScreenLocked () { return screenLocked; }
    //bool isAcceptingTouchInput();
    //bool isAcceptingKeyboardInput();
    //bool isAcceptingClicks ();
    //bool isAcceptingScrolls();

    //void refreshHighPriority (ViewManager* mgr);
    //void userInteracted ();

    void setInitialized (bool _initialized) {initialized = _initialized;}
    bool needsOnboarded () { return isClusterRoot; }
    bool joinCluster ();
    void setColorForStatus (ChatStatus chatStatus);
    void resetColor ();
  protected:
    //ChatterView currentView = ChatterViewSplash;
    //ChatterView lastView = ChatterViewSplash;
    ControlMessagingStatus messagingStatus = MessagingRunning;
    //bool initializeControlMode ();
    void processControlModeNotReady (ChatterUserEvent evt);
    bool busyShowing = false;

    //ViewManager* getCurrentViewManager ();
    void configureDeviceSettings();

    //ViewManager* viewManagers[MAX_VIEWS];
    //bool viewsReady = false;

    //UserInputManager* userInputManager;

    ControlMode* control = nullptr;
    ControlModeStatus status = ControlModeIdle;
    //ComponentRegistry* globalComponentRegistry;
    //ScreenRegistry* globalScreenRegistry;
    CallbackRegistry* globalCallbackRegistry;
    //GlobalCache* globalCache;

    //bool getStorageSemaphore ();
    //void returnStorageSemaphore ();

    //SemaphoreHandle_t* storageSemaphore;

    bool initialized = false;
    unsigned long messagingPausedUntil = 0;
    bool interruptedByUser = false;

    unsigned long lastUserInteraction = millis();
    unsigned long screenTimeout = 0; // replaced with setting after init

    SH1106Wire* display;

    //NewClusterForm newClusterForm;
    DeviceInitializationForm deviceInitializationForm;

    char lastDisplayLines[DISPLAY_NUM_LINES][DISPLAY_LINE_WIDTH];
    char displayLines[DISPLAY_NUM_LINES][DISPLAY_LINE_WIDTH];

    char displayRowBuffer[DISPLAY_LINE_WIDTH];
    void updateDashboard();
    void updateDisplay (const char* dispText, uint8_t line);
    void updateDisplay (const char lines[DISPLAY_NUM_LINES][DISPLAY_LINE_WIDTH]);
    void updateDisplay ();

    void updateTitle (const char* title);
    void updateSubtitle (const char* subtitle);
    void updateNeighbors ();
    void rotateDisplay ();

    TitleRotationItem currTitleItem = TitleAlias;
    unsigned long titleLastRotation = 0;
    char titleLine[32];

    const char* getStatusName (ChatStatus chatStatus);

    float meshCachePct = 0.0;
    float generalProgress = 0.0;

    float lastMeshCachePct = 0.0;
    float lastGeneralProgress = 0.0;

    bool isClusterRoot = true; // assume this is root until we are able to check
    bool ledRunning = false;
    bool displayRunning = false;

    ChatStatus lastChatStatus = ChatDisconnected;
    uint8_t lastNumNeighbors = 0;
    uint32_t lastNeighborsUpdate = 0;
};

#endif