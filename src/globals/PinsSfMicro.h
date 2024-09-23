#pragma once

#if defined(ARDUINO_TEENSY_MICROMOD) || defined(CORE_TEENSY)

/*
    // MM Teensy Processor Board
    int pin_cs = 10;
    int pin_dio0 = 4;
    int pin_tx_enable = 3;
    int pin_rx_enable = 40;
    int pin_nrst = 41;
    int pin_dio1 = 42;
*/

#define LORA_CS 10
#define LORA_INT 4 // dio0
#define LORA_RS 41
#define LORA_BUSY -1

//==========TEENSY==========
// We suggest flipping the switch to the G5 PROC and G6 PROC. The volrage regulators will work reguardless
// since SDIO_DATA2 and SDIO_DATA1 are connected regardless of what side the switch is on.
// Function Board 0's "PWR_EN0" pin <= MicroMod Main - Double SDIO_DATA2 => Teensy Processor Board (D39)
#define PWR_EN0 39
// Function Board 1's "PWR_EN1" pin <= MicroMod Main - Double SDIO_DATA1 => Teensy Processor Board (D34)
#define PWR_EN1 34

//#define TDECK_SPI_MOSI 41
//#define TDECK_SPI_MISO 38
//#define TDECK_SPI_SCK 40

// default value used in Teensy SD Example Code
//const int chipSelect = 10; 
// The microSD Card CS pin is D1 for the MicroMod Main Board - Single and Teensy Processor (5). Adjust for your processor if necessary.
//const int chipSelect 5;
// The microSD Card's CS pin is G4 for the MicroMod Main Board - Double and Teensy Processor (44). Adjust for your processor if necessary.
#define SDCARD_CS 44

//#define TDECK_BAT_ADC 4
#endif

