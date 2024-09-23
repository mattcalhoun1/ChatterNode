#include "src/globals/Globals.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "src/control/ControlMode.h"
#include "src/control/HeadlessControlMode.h"
#include "src/tasks/ControlLayer.h"
#include "ChatterAll.h"
#include "src/callbacks/CallbackRegistry.h"
#include "src/events/UserEvents.h"
#include "AlmostRandom.h"
#include "src/globals/TBeamBoard.h"

GpsEsp32RtClock* rtc;
//SPIClass SDCardSPI(HSPI);

CallbackRegistry* callbackRegistry;
ControlMode* control = nullptr;
ControlLayer* controlLayer;
UserEvents* userEvents;

enum ChatterStatus {
  ChatterUninitialized = 0,
  ChatterInitializing = 1,
  ChatterNoLicense = 2,
  ChatterAwaitingLicense = 3,
  ChatterError = 4,
  ChatterFactoryReset = 5,
  ChatterAlmostReady = 6,
  ChatterReady = 7
};

ChatterStatus currentChatterStatus = ChatterUninitialized;
bool initializing = true;
bool attemptedExternalRtc = false;
unsigned long homeShowTime = 0;

void setup() {
    // Open serial communications and wait for port to open:
    //Serial.begin(115200);
  
    // disable watchdogs (for now) since sd and radio usage have unpredictable delays
    // and i dont want to add ticks in the various libraries
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();

    AlmostRandom ar;
    randomSeed(ar.getRandomInt());

    setupBoard();

    setupLogging();

    rtc = new GpsEsp32RtClock(SerialGPS, 0, true, &Wire1);
    rtc->setDstEnabled(true);

    //setupPins();

    pinMode(SDCARD_CS, OUTPUT);
    pinMode(LORA_CS, OUTPUT);

    digitalWrite(SDCARD_CS, HIGH);
    digitalWrite(LORA_CS, HIGH);

    //Wire.begin(17, 18);
    //Wire1.begin(42, 41);

    //setupGps();

    //delay(500);

    userEvents = new UserEvents();
    callbackRegistry = new CallbackRegistry();
    controlLayer = new ControlLayer(callbackRegistry);
}
// startup
// 1. power up all hardware
// 2. wait for rtc
// 3. Try to initialize chatter
//   3.a if Chatter initializes, run standard loop for ever
//   3.b if Chatter fails to initialize...
//      3.b.1 generate / save device id
//      3.b.2 generate new cluster and identity (b.[rand])
//      3.b.3 wait forever to be onboarded
//   3.b if Chatter initializes but is root of its own cluster, wait forever to be onboarded
// 


void loop() 
{
    if (initializing) {
        startChatterNextStep();
        delay(10);
    }
    else if (controlLayer->needsOnboarded()) {
        // go into onboard mode
        Logger::info("Want to onboard..", LogAppControl);
        controlLayer->joinCluster();
    }

    if (controlLayer != nullptr) {
        controlLayer->process(userEvents->dequeueNextEvent());
    }
    else {
        Logger::error("ctrl pointer is null", LogAppControl);
    }
}

// attempt to start chatter layer, when everything is read
void startChatterNextStep () {
  // first, GPS/rtc must be setup
  if (isRtcReady()) {
    if (currentChatterStatus == ChatterError) {
        Logger::error("Error during init!", LogAppControl);
        while (true) {delay(10);}
    }
    else if (currentChatterStatus == ChatterUninitialized) {
      // begin the chatter initialization
      Logger::debug("Awaiting chatter initialization ", LogAppControl);

      // give control layer pointer to control mode so it can take
      // over on the other cpu
      if (control == nullptr) {

        control = new HeadlessControlMode(DeviceTypeBase, rtc, callbackRegistry, SDCARD_CS, SDCardSPI);
      }


      if (control->openStorage()) {
        controlLayer->setControlMode(control);

        currentChatterStatus = ChatterInitializing;
      }
      else {
        Logger::error("Insert valid SD card...", LogAppControl);
        delay(3000);
      }

    }
    else if (currentChatterStatus == ChatterInitializing) {
      // run next step of init. The only steps we need to do something
      // here on are terminal steps
      ControlModeStatus controlStatus = controlLayer->initializeNextStep();
      switch (controlStatus) {
        case ControlStartupUnlicensed:
          currentChatterStatus = ChatterNoLicense;
          Logger::info("No License", LogLicensing);
          controlLayer->setInitialized(true); 
          break;
        case ControlStartupInitializeDevice:
          currentChatterStatus = ChatterFactoryReset;
          Logger::warn("Need user init", LogAppControl);
          break;
        case ControlStartupError:
        case ControlModeError:
          currentChatterStatus = ChatterError;
          Logger::debug("Startup error! Might need to wipe SD card", LogAppControl);

            while (true) {
                delay(10);
            }

          //controlLayer->setInitialized(true); 
          break;
        case ControlStartupNeedPassword:
          // the user will be prompted again
          //Logger::info("control layer needs password", LogAppControl);
          break;
        case ControlModeReady:
          Logger::debug("Control mode almost ready!", LogAppControl);
          currentChatterStatus = ChatterAlmostReady;
          break;
        default:
            break;
      }
    }
    else if (currentChatterStatus == ChatterNoLicense) {
      char spacedDeviceId[24];
      memset(spacedDeviceId, 0, 24);

      const char* rawHardwareId = control->getChatter()->getUniqueHardwareId();
      Logger::info("Hardware ID from chatter: ", rawHardwareId, LogLicensing);

      // divide the license into 4s
      uint8_t wordSize = 0;
      uint8_t wordCount = 0;
      for (uint8_t c = 0; c < strlen(rawHardwareId); c++) {
        spacedDeviceId[c + wordCount] = rawHardwareId[c];
        wordSize++;

        if (wordSize >= 4) {
          wordSize = 0;
          wordCount += 1;
          spacedDeviceId[c + wordCount] = ' ';
        }
      }

      currentChatterStatus = ChatterAwaitingLicense;
      Logger::info("device has no license", LogLicensing);
    }
    else if (currentChatterStatus == ChatterAwaitingLicense) {
      // do nothing unless one has been received
      //Logger::debug("awaiting license...", LogLicensing);
    }
    else if (currentChatterStatus == ChatterFactoryReset) {
      Logger::warn("Going through reset", LogAppControl);
    }
    else if (currentChatterStatus == ChatterAlmostReady) {
      // allow half a second for the password prompt to go away if it needs to
      if (homeShowTime == 0) {
        homeShowTime = millis() + 500;
      }
      else if (homeShowTime < millis()) {
        currentChatterStatus = ChatterReady;
        controlLayer->setInitialized(true); 
      }

    }
    else if (currentChatterStatus == ChatterReady) {
        initializing = false;
        // queue event showing transition to home
        userEvents->queueEvent(ViewChangeHome);
        control->hideChatProgress();

    }
  }
}

bool isRtcReady () {
  if (!rtc->hasSignalBeenAcquired()) {
    controlLayer->updateChatViewStatus("Acquiring GPS Sig");
    controlLayer->updateChatViewProgress(.1);
    Logger::debug("Attempting to acquire gps signal", LogLocation);
    if (rtc->acquireSignal (500)) {
      Logger::info("Signal acquired", LogLocation);
    }
    else if (attemptedExternalRtc == false) {
      attemptedExternalRtc = true;
      if (rtc->attemptExternalRtcTimeSync()) {
        Logger::info("Time via external RTC", LogRtc);
      }
    }
    else if (rtc->attemptSerialTimeSync (500)) {
      Logger::info("Time via serial", LogRtc);
    }
  }
  else if (!rtc->hasTimeBeenSynced()) {
    controlLayer->updateChatViewStatus("RTC: Syncing");
    controlLayer->updateChatViewProgress(.2);
    if(rtc->syncWithExternalRtc()) {
      Logger::info("RTC synced with gps", LogRtc);
    }
    else {
      Logger::warn("RTC sync failed!", LogRtc);
    }
  }
  else if (initializing) {
    controlLayer->updateChatViewStatus("Decrypting Storage");
    controlLayer->updateChatViewProgress(.7);
    return true;
  }

  return false;
}


void setupLogging () {
  unsigned long serialWait = 1000;
  unsigned long serialStartTime = millis();
  bool serialConnected = true;
  while (!Serial) {
    if (millis() - serialStartTime > serialWait) {
      serialConnected = false;
      break;
    }
  }

  if (serialConnected) {
    Logger::init(new SerialLogger());
    Logger::setLogLevel (LogChatter, LogLevelDebug);
    Logger::setLogLevel (LogStorage, LogLevelDebug);
    Logger::setLogLevel (LogEncryption, LogLevelInfo);
    Logger::setLogLevel (LogComLora, LogLevelDebug);
    Logger::setLogLevel (LogComWiFi, LogLevelInfo);
    Logger::setLogLevel (LogComWired, LogLevelInfo);
    Logger::setLogLevel (LogMesh, LogLevelInfo);
    Logger::setLogLevel (LogMeshStrategy, LogLevelInfo);
    Logger::setLogLevel (LogLicensing, LogLevelInfo);
    Logger::setLogLevel (LogUi, LogLevelDebug);
    Logger::setLogLevel (LogUiEvents, LogLevelInfo);
    Logger::setLogLevel (LogAppControl, LogLevelInfo);
    Logger::setLogLevel (LogMeshGraph, LogLevelInfo);
    Logger::setLogLevel (LogLocation, LogLevelInfo);
    Logger::setLogLevel (LogRtc, LogLevelDebug);
    Logger::setLogLevel (LogOnboard, LogLevelInfo);
    Logger::setLogLevel (LogInit, LogLevelInfo);
    Logger::setLogLevel (LogBackup, LogLevelInfo); 
  }
  else {
    Logger::init(new NullLogger());
  }
}