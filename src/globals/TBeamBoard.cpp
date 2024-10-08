#include "TBeamBoard.h"

#if defined(HAS_SDCARD)
SPIClass SDCardSPI(HSPI);
#endif


#if defined(ARDUINO_ARCH_STM32)
HardwareSerial  SerialGPS(GPS_RX_PIN, GPS_TX_PIN);
#endif

#if defined(ARDUINO_ARCH_ESP32)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
#include "hal/gpio_hal.h"
#endif
#include "driver/gpio.h"
#endif //ARDUINO_ARCH_ESP32


static DevInfo_t  devInfo;

#ifdef HAS_GPS
static bool find_gps = false;
#endif



#ifdef HAS_PMU
XPowersLibInterface *PMU = NULL;
bool     pmuInterrupt;

static void setPmuFlag()
{
    pmuInterrupt = true;
}
#endif

bool beginPower()
{
#ifdef HAS_PMU
    if (!PMU) {
        PMU = new XPowersAXP2101(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Logger::warn("Warning: Failed to find AXP2101 power management", LogAppControl);
            delete PMU;
            PMU = NULL;
        } else {
            Logger::info("AXP2101 PMU init succeeded, using AXP2101 PMU", LogAppControl);
        }
    }

    if (!PMU) {
        PMU = new XPowersAXP192(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Logger::info("Warning: Failed to find AXP192 power management", LogAppControl);
            delete PMU;
            PMU = NULL;
        } else {
            Logger::info("AXP192 PMU init succeeded, using AXP192 PMU", LogAppControl);
        }
    }

    if (!PMU) {
        return false;
    }

    PMU->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

    pinMode(PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PMU_IRQ, setPmuFlag, FALLING);

    if (PMU->getChipModel() == XPOWERS_AXP192) {

        PMU->setProtectedChannel(XPOWERS_DCDC3);

        // lora
        PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        // gps
        PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        // oled
        PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);

        PMU->enablePowerOutput(XPOWERS_LDO2);
        PMU->enablePowerOutput(XPOWERS_LDO3);

        //protected oled power source
        PMU->setProtectedChannel(XPOWERS_DCDC1);
        //protected esp32 power source
        PMU->setProtectedChannel(XPOWERS_DCDC3);
        // enable oled power
        PMU->enablePowerOutput(XPOWERS_DCDC1);

        //disable not use channel
        PMU->disablePowerOutput(XPOWERS_DCDC2);

        PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);

        PMU->enableIRQ(XPOWERS_AXP192_VBUS_REMOVE_IRQ |
                       XPOWERS_AXP192_VBUS_INSERT_IRQ |
                       XPOWERS_AXP192_BAT_CHG_DONE_IRQ |
                       XPOWERS_AXP192_BAT_CHG_START_IRQ |
                       XPOWERS_AXP192_BAT_REMOVE_IRQ |
                       XPOWERS_AXP192_BAT_INSERT_IRQ |
                       XPOWERS_AXP192_PKEY_SHORT_IRQ
                      );

    } else if (PMU->getChipModel() == XPOWERS_AXP2101) {

#if defined(CONFIG_IDF_TARGET_ESP32)
        //Unuse power channel
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        PMU->disablePowerOutput(XPOWERS_DCDC3);
        PMU->disablePowerOutput(XPOWERS_DCDC4);
        PMU->disablePowerOutput(XPOWERS_DCDC5);
        PMU->disablePowerOutput(XPOWERS_ALDO1);
        PMU->disablePowerOutput(XPOWERS_ALDO4);
        PMU->disablePowerOutput(XPOWERS_BLDO1);
        PMU->disablePowerOutput(XPOWERS_BLDO2);
        PMU->disablePowerOutput(XPOWERS_DLDO1);
        PMU->disablePowerOutput(XPOWERS_DLDO2);

        // GNSS RTC PowerVDD 3300mV
        PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
        PMU->enablePowerOutput(XPOWERS_VBACKUP);

        //ESP32 VDD 3300mV
        // ! No need to set, automatically open , Don't close it
        // PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
        // PMU->setProtectedChannel(XPOWERS_DCDC1);
        PMU->setProtectedChannel(XPOWERS_DCDC1);

        // LoRa VDD 3300mV
        PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO2);

        //GNSS VDD 3300mV
        PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO3);

#endif /*CONFIG_IDF_TARGET_ESP32*/


        /** tbeam supreme settings **/

        //t-beam m.2 inface
        //gps
        PMU->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO4);

        // lora
        PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO3);

        // In order to avoid bus occupation, during initialization, the SD card and QMC sensor are powered off and restarted
        if (ESP_SLEEP_WAKEUP_UNDEFINED == esp_sleep_get_wakeup_cause()) {
            Logger::info("Power off and restart ALDO BLDO..", LogAppControl);
            PMU->disablePowerOutput(XPOWERS_ALDO1);
            PMU->disablePowerOutput(XPOWERS_ALDO2);
            PMU->disablePowerOutput(XPOWERS_BLDO1);
            delay(250);
        }

        // Sensor
        PMU->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO1);

        PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO2);

        //Sdcard

        PMU->setPowerChannelVoltage(XPOWERS_BLDO1, 3300);
        PMU->enablePowerOutput(XPOWERS_BLDO1);

        PMU->setPowerChannelVoltage(XPOWERS_BLDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_BLDO2);

        //face m.2
        PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
        PMU->enablePowerOutput(XPOWERS_DCDC3);

        PMU->setPowerChannelVoltage(XPOWERS_DCDC4, XPOWERS_AXP2101_DCDC4_VOL2_MAX);
        PMU->enablePowerOutput(XPOWERS_DCDC4);

        PMU->setPowerChannelVoltage(XPOWERS_DCDC5, 3300);
        PMU->enablePowerOutput(XPOWERS_DCDC5);


        //not use channel
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        // PMU->disablePowerOutput(XPOWERS_DCDC4);
        // PMU->disablePowerOutput(XPOWERS_DCDC5);
        PMU->disablePowerOutput(XPOWERS_DLDO1);
        PMU->disablePowerOutput(XPOWERS_DLDO2);
        PMU->disablePowerOutput(XPOWERS_VBACKUP);

        // -- end tbeam supreme-specific settings

        // Set constant current charge current limit
        PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

        // Set charge cut-off voltage
        PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

        // Disable all interrupts
        PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
        // Clear all interrupt flags
        PMU->clearIrqStatus();
        // Enable the required interrupt function
        PMU->enableIRQ(
            XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ      |   //BATTERY
            XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ     |   //VBUS
            XPOWERS_AXP2101_PKEY_SHORT_IRQ    | XPOWERS_AXP2101_PKEY_LONG_IRQ       |   //POWER KEY
            XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ       //CHARGE
            // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
        );

    }

    PMU->enableSystemVoltageMeasure();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();

    /*
    Serial.printf("=========================================\n");
    if (PMU->isChannelAvailable(XPOWERS_DCDC1)) {
        Serial.printf("DC1  : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC1));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC2)) {
        Serial.printf("DC2  : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC2));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC3)) {
        Serial.printf("DC3  : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC3)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC3));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC4)) {
        Serial.printf("DC4  : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC4)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC4));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC5)) {
        Serial.printf("DC5  : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC5)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC5));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO2)) {
        Serial.printf("LDO2 : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_LDO2)   ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_LDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO3)) {
        Serial.printf("LDO3 : %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_LDO3)   ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_LDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO1)) {
        Serial.printf("ALDO1: %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO2)) {
        Serial.printf("ALDO2: %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO3)) {
        Serial.printf("ALDO3: %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO3)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO4)) {
        Serial.printf("ALDO4: %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO4)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO4));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
        Serial.printf("BLDO1: %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_BLDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_BLDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
        Serial.printf("BLDO2: %s   Voltage: %04u mV \n",  PMU->isPowerChannelEnable(XPOWERS_BLDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_BLDO2));
    }
    Serial.printf("=========================================\n");
    */

    // Set the time of pressing the button to turn off
    PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    uint8_t opt = PMU->getPowerKeyPressOffTime();
    switch (opt) {
    case XPOWERS_POWEROFF_4S: Logger::info("PowerKeyPressOffTime:", "4 Second", LogAppControl);
        break;
    case XPOWERS_POWEROFF_6S: Logger::info("PowerKeyPressOffTime:", "6 Second", LogAppControl);
        break;
    case XPOWERS_POWEROFF_8S: Logger::info("PowerKeyPressOffTime:", "8 Second", LogAppControl);
        break;
    case XPOWERS_POWEROFF_10S: Logger::info("PowerKeyPressOffTime:", "10 Second", LogAppControl);
        break;
    default:
        break;
    }
#endif
    return true;
}

void disablePeripherals()
{

#ifdef HAS_PMU
    if (!PMU)return;
#if defined(T_BEAM_S3_BPF)
    PMU->disablePowerOutput(XPOWERS_ALDO4); //gps
    PMU->disablePowerOutput(XPOWERS_ALDO2); //Sdcard
    PMU->disablePowerOutput(XPOWERS_DCDC3); // Extern Power source
    PMU->disablePowerOutput(XPOWERS_DCDC5);
    PMU->disablePowerOutput(XPOWERS_ALDO1);
#endif
#endif


}


void loopPMU()
{
#ifdef HAS_PMU
    if (!PMU) {
        return;
    }
    if (!pmuInterrupt) {
        return;
    }

    pmuInterrupt = false;
    // Get PMU Interrupt Status Register
    uint32_t status = PMU->getIrqStatus();

    if (PMU->isVbusInsertIrq()) {
        Logger::info("isVbusInsert", LogAppControl);
    }
    if (PMU->isVbusRemoveIrq()) {
        Logger::info("isVbusRemove", LogAppControl);
    }
    if (PMU->isBatInsertIrq()) {
        Logger::info("isBatInsert", LogAppControl);
    }
    if (PMU->isBatRemoveIrq()) {
        Logger::info("isBatRemove", LogAppControl);
    }
    if (PMU->isPekeyShortPressIrq()) {
        Logger::info("isPekeyShortPress", LogAppControl);
    }
    if (PMU->isPekeyLongPressIrq()) {
        Logger::info("isPekeyLongPress", LogAppControl);
    }
    if (PMU->isBatChagerDoneIrq()) {
        Logger::info("isBatChagerDone", LogAppControl);
    }
    if (PMU->isBatChagerStartIrq()) {
        Logger::info("isBatChagerStart", LogAppControl);
    }
    // Clear PMU Interrupt Status Register
    PMU->clearIrqStatus();
#endif
}


void printWakeupReason()
{
#ifdef ESP32
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        Logger::info(" In case of deep sleep, reset was not caused by exit from deep sleep", LogAppControl);
        break;
    case ESP_SLEEP_WAKEUP_ALL :
        break;
    case ESP_SLEEP_WAKEUP_EXT0 :
        Logger::info("Wakeup caused by external signal using RTC_IO", LogAppControl);
        break;
    case ESP_SLEEP_WAKEUP_EXT1 :
        Logger::info("Wakeup caused by external signal using RTC_CNTL", LogAppControl);
        break;
    case ESP_SLEEP_WAKEUP_TIMER :
        Logger::info("Wakeup caused by timer", LogAppControl);
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD :
        Logger::info("Wakeup caused by touchpad", LogAppControl);
        break;
    case ESP_SLEEP_WAKEUP_ULP :
        Logger::info("Wakeup caused by ULP program", LogAppControl);
        break;
    default :
        Logger::info("Wakeup was not caused by deep sleep", LogAppControl);
        break;
    }
#endif
}


void getChipInfo()
{
    printWakeupReason();

#if defined(CONFIG_IDF_TARGET_ESP32)  ||  defined(CONFIG_IDF_TARGET_ESP32S3)

    if (psramFound()) {
        uint32_t psram = ESP.getPsramSize();
        devInfo.psramSize = psram / 1024.0 / 1024.0;
        Logger::info("PSRAM is enabled!", LogAppControl);
    } else {
        Logger::info("PSRAM is disabled!", LogAppControl);
        devInfo.psramSize = 0;
    }

#endif

    devInfo.flashSize       = ESP.getFlashChipSize() / 1024.0 / 1024.0;
    devInfo.flashSpeed      = ESP.getFlashChipSpeed() / 1000 / 1000;
    devInfo.chipModel       = ESP.getChipModel();
    devInfo.chipModelRev    = ESP.getChipRevision();
    devInfo.chipFreq        = ESP.getCpuFreqMHz();


}



void setupBoard()
{

    getChipInfo();

#if defined(ARDUINO_ARCH_ESP32)
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
#elif defined(ARDUINO_ARCH_STM32)
    SPI.setMISO(RADIO_MISO_PIN);
    SPI.setMOSI(RADIO_MOSI_PIN);
    SPI.setSCLK(RADIO_SCLK_PIN);
    SPI.begin();
#endif

#ifdef HAS_SDCARD
    SDCardSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI);
#endif

#ifdef I2C_SDA
    Wire.begin(I2C_SDA, I2C_SCL);
    //scanDevices(&Wire);
#endif

#ifdef I2C1_SDA
    Wire1.begin(I2C1_SDA, I2C1_SCL);
#endif

#ifdef HAS_GPS
#if defined(ARDUINO_ARCH_ESP32)
    SerialGPS.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#elif defined(ARDUINO_ARCH_STM32)
    SerialGPS.setRx(GPS_RX_PIN);
    SerialGPS.setTx(GPS_TX_PIN);
    SerialGPS.begin(GPS_BAUD_RATE);
#endif // ARDUINO_ARCH_
#endif // HAS_GPS

#if OLED_RST
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, HIGH); delay(20);
    digitalWrite(OLED_RST, LOW);  delay(20);
    digitalWrite(OLED_RST, HIGH); delay(20);
#endif

#ifdef BOARD_LED
    /*
    * T-Beam LED defaults to low level as turn on,
    * so it needs to be forced to pull up
    * * * * */
#if LED_ON == LOW
#if defined(ARDUINO_ARCH_ESP32)
    gpio_hold_dis((gpio_num_t)BOARD_LED);
#endif //ARDUINO_ARCH_ESP32
#endif

    pinMode(BOARD_LED, OUTPUT);
    digitalWrite(BOARD_LED, LED_ON);
#endif


#if defined(ARDUINO_ARCH_STM32)
    SerialGPS.println("@GSR"); delay(300);
    SerialGPS.println("@GSR"); delay(300);
    SerialGPS.println("@GSR"); delay(300);
    SerialGPS.println("@GSR"); delay(300);
    SerialGPS.println("@GSR"); delay(300);
#endif

/*
#ifdef RADIO_LDO_EN
    pinMode(RADIO_LDO_EN, OUTPUT);
    digitalWrite(RADIO_LDO_EN, HIGH);
#endif
*/
    beginPower();

    //beginSDCard();

    /*if (!disable_u8g2) {
        beginDisplay();
    }

    beginWiFi();
    */
    Logger::info("init done . ", LogAppControl);
}



static uint8_t ledState = LOW;
static const uint32_t debounceDelay = 50;
static uint32_t lastDebounceTime = 0;

void flashLed()
{
#ifdef BOARD_LED
    if ((millis() - lastDebounceTime) > debounceDelay) {
        ledState = !ledState;
        if (ledState) {
            digitalWrite(BOARD_LED, LED_ON);
        } else {
            digitalWrite(BOARD_LED, !LED_ON);
        }
        lastDebounceTime = millis();
    }
#endif
}

#ifdef HAS_GPS
bool beginGPS()
{
    SerialGPS.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    bool result = false;
    uint32_t startTimeout ;
    for (int i = 0; i < 3; ++i) {
        SerialGPS.write("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
        delay(5);
        // Get version information
        startTimeout = millis() + 3000;
        Logger::info("Try to init L76K . Wait stop .", LogAppControl);
        while (SerialGPS.available()) {
            SerialGPS.readString();
            if (millis() > startTimeout) {
                Logger::info("Wait L76K stop NMEA timeout!", LogAppControl);
                return false;
            }
        };
        SerialGPS.flush();
        delay(200);

        SerialGPS.write("$PCAS06,0*1B\r\n");
        startTimeout = millis() + 500;
        String ver = "";
        while (!SerialGPS.available()) {
            if (millis() > startTimeout) {
                Logger::info("Get L76K timeout!", LogAppControl);
                return false;
            }
        }
        SerialGPS.setTimeout(10);
        ver = SerialGPS.readStringUntil('\n');
        if (ver.startsWith("$GPTXT,01,01,02")) {
            Logger::info("L76K GNSS init succeeded, using L76K GNSS Module", LogAppControl);
            result = true;
            break;
        }
        delay(500);
    }
    // Initialize the L76K Chip, use GPS + GLONASS
    SerialGPS.write("$PCAS04,5*1C\r\n");
    delay(250);
    // only ask for RMC and GGA
    SerialGPS.write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    SerialGPS.write("$PCAS11,3*1E\r\n");
    return result;
}
#endif