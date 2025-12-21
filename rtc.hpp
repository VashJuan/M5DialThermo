/**
 * @file rtc.hpp
 * @brief Real-Time Clock Header
 * @version 1.0
 * @date 2025-12-21
 *
 * WS1850S is the internal Real-Time-Clock in the M5Dial
 * https://shop.m5stack.com/products/rfid-unit-2-ws1850s
 * or uses NXP's PCF8563 RTC https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf
 * https://docs.m5stack.com/en/arduino/m5unified/rtc8563_class
 *
 * This code is directly from https://docs.m5stack.com/en/arduino/m5dial/rtc
 */

#ifndef RTC_HPP
#define RTC_HPP

#include <M5Dial.h>
#include <WiFi.h>
#include "secrets.h"

// Different versions of the framework have different SNTP header file names and availability.
#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define SNTP_ENABLED 1
#elif __has_include(<sntp.h>)
#include <sntp.h>
#define SNTP_ENABLED 1
#endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

// NTP Configuration
#define NTP_TIMEZONE "UTC-8" // POSIX standard, in which "UTC+0" is UTC London, "UTC+8" is UTC+8 Seattle
#define NTP_SERVER1 "0.pool.ntp.org"
#define NTP_SERVER2 "1.pool.ntp.org"
#define NTP_SERVER3 "2.pool.ntp.org"

// Weekday strings
static constexpr const char *const wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};

// Forward declarations
extern M5Dial m5Dial;

// RTC class definition would go here in the future
// For now, using M5Dial's built-in RTC functionality

// Function declarations
void setup();
void loop();

// Global RTC instance
class RTC
{
public:
    // Placeholder for future RTC class implementation
    // Currently using M5Dial.Rtc directly
};

extern RTC rtc;

#endif // RTC_HPP