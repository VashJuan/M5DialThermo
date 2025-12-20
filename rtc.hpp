/**
 * @file rtc.hpp
 * @brief M5Dial RTC Class Header File
 * @version 0.2
 * @date 2025-12-16
 *
 * @Hardwares: M5Dial
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#pragma once

#include <Arduino.h>
#include <time.h>
#include "M5Dial.h"

#if defined(ARDUINO)
#include <WiFi.h>

// Different versions of the framework have different SNTP header file names and
// availability.
#if __has_include(<esp_sntp.h>)
#include <esp_sntp.h>
#define SNTP_ENABLED 1
#elif __has_include(<sntp.h>)
#include <sntp.h>
#define SNTP_ENABLED 1
#endif

#endif

#ifndef SNTP_ENABLED
#define SNTP_ENABLED 0
#endif

/**
 * @struct WiFiConfig
 * @brief WiFi configuration structure
 */
struct WiFiConfig
{
    const char *ssid;
    const char *password;
};

/**
 * @struct NTPConfig
 * @brief NTP configuration structure
 */
struct NTPConfig
{
    const char *timezone;
    const char *server1;
    const char *server2;
    const char *server3;
};

/**
 * @class RTC
 * @brief RTC class for M5Dial device with NTP synchronization
 *
 * This class provides functionality to handle RTC operations, WiFi connectivity,
 * NTP synchronization, and time display for the M5Dial device.
 */
class RTC
{
private:
    WiFiConfig wifiConfig;
    NTPConfig ntpConfig;
    bool isInitialized;

    /**
     * @brief Connect to WiFi using stored credentials
     * @return true if connected successfully
     */
    bool connectToWiFi();

    /**
     * @brief Synchronize time via NTP
     * @return true if synchronization successful
     */
    bool synchronizeNTP();

public:
    /**
     * @brief Constructor with default configuration
     */
    RTC();

    /**
     * @brief Constructor with custom WiFi and NTP configuration
     * @param wifi WiFi configuration
     * @param ntp NTP configuration
     */
    RTC(const WiFiConfig &wifi, const NTPConfig &ntp);

    /**
     * @brief Destructor
     */
    ~RTC();

    /**
     * @brief Setup RTC with NTP synchronization
     * Initializes RTC, connects to WiFi, and synchronizes time via NTP
     * @return true if setup successful
     */
    bool setup();

    /**
     * @brief Update and display current time information
     * Displays current time from RTC and ESP32 internal timer
     */
    void update();

    /**
     * @brief Get current time from system
     * @return time_t Current time, or 0 if failed
     */
    time_t getCurrentTime();

    /**
     * @brief Get RTC date and time
     * @return M5.RTC_DateTimeType RTC date and time structure
     */
    auto getRTCDateTime() -> decltype(M5Dial.Rtc.getDateTime());

    /**
     * @brief Set RTC date and time manually
     * @param dateTime Date and time structure to set
     */
    void setDateTime(const m5::rtc_datetime_t &dateTime);

    /**
     * @brief Check if RTC is enabled
     * @return true if RTC is enabled
     */
    bool isRTCEnabled();

    /**
     * @brief Set WiFi credentials
     * @param ssid WiFi SSID
     * @param password WiFi password
     */
    void setWiFiCredentials(const char *ssid, const char *password);

    /**
     * @brief Set NTP configuration
     * @param timezone Timezone string
     * @param server1 Primary NTP server
     * @param server2 Secondary NTP server
     * @param server3 Tertiary NTP server
     */
    void setNTPConfig(const char *timezone, const char *server1,
                      const char *server2 = nullptr, const char *server3 = nullptr);

    /**
     * @brief Format time for display
     * @param t Time structure
     * @param includeWeekday Include weekday in format
     * @return Formatted time string
     */
    String formatTime(const struct tm *t, bool includeWeekday = true);

    /**
     * @brief Check if system is initialized
     * @return true if initialized
     */
    bool isSystemInitialized() const;
};

// Global instance for easy access
extern RTC rtc;