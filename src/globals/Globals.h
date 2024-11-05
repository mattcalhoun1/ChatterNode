#ifndef GLOBALS_H

#define GLOBALS_H

#define CONFIG_ESP_TASK_WDT_TIMEOUT_S 1000

#include "PinsTBeam.h"

//#define INCLUDE_vTaskSuspend 1 // wait indefinitely for semaphore

#define CHATTERBOX_FIRMWARE_VERSION "1.0.3"
#define STRONG_ENCRYPTION_ENABLED true // false for export

#define RH_SX126x_MAX_MESSAGE_LEN 150
#define RH_BROADCAST_ADDRESS 255

#define RH_HAVE_HARDWARE_SPI true
#define RH_PLATFORM RH_PLATFORM_ESP32
#define RH_ESP32_USE_HSPI true
#define SPI_HAS_TRANSACTION true

/*
#define LOG_DEBUG
#define LOG_INFO
#define LOG_WARN
#define LOG_ERROR
#define LOG_ANALYSIS
*/

//#define RH_RADIOLIB_HEADER_LEN 4
//#ifndef RH_RADIOLIB_MAX_MESSAGE_LEN
//#define RH_RADIOLIB_MAX_PAYLOAD_LEN 255
//#define RH_RADIOLIB_MAX_MESSAGE_LEN (RH_RADIOLIB_MAX_PAYLOAD_LEN - RH_RADIOLIB_HEADER_LEN)
//#endif

#define BASE_DEFAULT_CENTER_FREQ "915.0"
#define BASE_DEFAULT_NUM_CHANNELS "64"
#define BASE_DEFAULT_HOP_SCHEDULE "100"

// L76K GPS USE 9600 BAUDRATE
//#define GPS_BAUD        9600

// M10Q GPS USE 38400 BAUDRATE
// #define GPS_BAUD        38400


// which control mode in use
//#define CONTROL_MODE_GUI true
//#define CONTROL_MODE_TEST false
//#define FCC_TEST_ENABLED true

#define LORA_DEFAULT_FREQUENCY LORA_DEFAULT_FREQUENCY_915

#define BACKPACK_THERMAL_ENABLED false
#define BACKPACK_RELAY_ENABLED true
#define MAX_BACKPACKS 2

//#define RH_PLATFORM 1
#define APP_TITLE "ChatterBox"
#define APP_SUBTITLE ">> Chatters Secure Messaging <<"

#define DEFAULT_DEVICE_ALIAS "Com"

#define CHATTER_LICENSING_SITE_PREFIX "https://api.chatters.io/ChatterLicenseGenerator?devid="

/*#define BRIDGE_LORA_ALIAS "Bridge_LoRa"
#define BRIDGE_WIFI_ALIAS "Bridge_Wifi"
#define BRIDGE_CLOUD_ALIAS "Bridge_Cloud"*/

#define MAX_CHANNELS 2 // how many can be simultaneously monitored at once
#define CHANNEL_DISPLAY_SIZE 32 // how many chars the channel name + config can occupy for display purposes

// how long for ui ot wait for initial connect
#define CLUSTER_ONBOARD_TIMEOUT 30000

#define COMMUNICATOR_MESSAGE_BUFFER_SIZE 128

// wifi not in use, by default
#define WIFI_SSID_MAX_LEN 24
#define WIFI_CRED_MAX_LEN 24

#define CHATTER_LORA_ENABLED // lora enabled on all devices (needed for onboarding)
#define CHATTER_UART_ENABLED // uart enabled on all devices (if user turns on pref)
#define BACKPACK_ENABLED false // backpacks not yet supported, would share same port as uart

// size of full message buffer within ui (combined decrypted packets)
#define GUI_MESSAGE_BUFFER_SIZE 1025
#define GUI_MAX_MESSAGE_LENGTH 1024

// bridging-related settings
// any message sitting in an out queue older thna this is subject to pruning
// adjust this to keep bridges moving
#define MAX_MESSAGE_AGE_SECONDS 90
#define MESSAGE_PRUNE_FREQUENCY 100

// rtc syncing
#define RTC_SYNC_ENABLED false // once the device has booted, whether to keep syncing. it has caused freeze issues on some devices
#define RTC_SYNC_FREQUENCY 1000 // how often to sync rtc clock, the onboard one can drift. 1000 cycles is roughly every 15 min

// Choose SPI or I2C fram chip (SPI is faster supports larger sizes)
#define STORAGE_SD_CARD true
//#define STORAGE_FRAM_SPI true
//#define STORAGE_FRAM_I2C true

// Currently only adafruit m4 is supported
/*
#define LORA_RFM9X_CS A4
#define LORA_RFM9X_INT A5
#define LORA_RFM9X_RST A0
#define LORA_RFM9X_BUSY -1 // no busy pin
*/

/*
#define CHATTER_WIFI_ENABLED
#define ESP32_RESETN A3  // was btn a
#define SPIWIFI_SS A1 // was rotary 1
#define SPIWIFI_ACK A2 // was rotary 2 // a.k.a BUSY or READY
#define ESP32_GPIO0   -1  // Not connected

*/

// for thermal cam, need interpolation settings and buffer
/*#define THERMAL_INTERPOLATE_BUFFER_SIZE 768
#define THERMAL_INTERPOLATE_LEVEL 2 // Each interpolation level (max 2) doubles image size, requiring larger interpolation buffer
#define THERMAL_ENCODE_BUFFER_SIZE 768
#define THERMAL_WIDTH 32
#define THERMAL_HEIGHT 24
#define THERMAL_INTERPOLATED_WIDTH 64
#define THERMAL_INTERPOLATED_HEIGHT 48

//#include "Globals_Colors.h"

#define QR_CODE_SIZE 170
*/

#endif