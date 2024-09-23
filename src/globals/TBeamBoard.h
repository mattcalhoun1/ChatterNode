#pragma once

#include "PinsTBeam.h"
#include "ChatterLogging.h"

#ifdef HAS_SDCARD
#include <SD.h>
#endif

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <XPowersLib.h>

#ifndef PMU_WIRE_PORT
#define PMU_WIRE_PORT           Wire
#endif

typedef struct {
    String          chipModel;
    float           psramSize;
    uint8_t         chipModelRev;
    uint8_t         chipFreq;
    uint8_t         flashSize;
    uint8_t         flashSpeed;
} DevInfo_t;


void setupBoard();
void disablePeripherals();
bool beginPower();
void flashLed();
bool beginGPS();
void loopPMU();

#ifdef HAS_PMU
extern XPowersLibInterface *PMU;
extern bool pmuInterrupt;
#endif

#if defined(ARDUINO_ARCH_ESP32)

#if defined(HAS_SDCARD)
extern SPIClass SDCardSPI;
#endif

#define SerialGPS Serial1
#elif defined(ARDUINO_ARCH_STM32)
extern HardwareSerial  SerialGPS;
#endif