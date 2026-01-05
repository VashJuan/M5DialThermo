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

#include <M5Unified.h>

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
    String fallbackTimezone;

    /**
     * @brief Load fallback timezone from temps.csv
     * @return true if successfully loaded
     */
    bool loadFallbackTimezone();

    /**
     * @brief Detect timezone automatically using IP geolocation
     * @return true if timezone was detected successfully
     */
    bool detectTimezoneFromLocation();

    /**
     * @brief Convert UTC offset and timezone name to ESP32 format
     * @param utcOffset UTC offset string (e.g., "+08:00")
     * @param timezoneName Timezone name for reference
     * @return ESP32 timezone string (e.g., "UTC-8")
     */
    String convertToESP32Timezone(const String &utcOffset, const String &timezoneName);

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

    /**
     * @brief Try alternative NTP synchronization method
     * @return true if synchronization successful
     */
    bool tryAlternativeNTPSync();

    /**
     * @brief Set RTC hardware from NTP-synchronized time
     * @return true if RTC was set successfully
     */
    bool setRTCFromNTP();

    /**
     * @brief Test DNS connectivity before attempting NTP sync
     * @return true if DNS is working
     */
    bool testDNSConnectivity();

    /**
     * @brief Report HTTP error codes with descriptive messages
     * @param errorCode HTTP error code to report
     */
    void reportHTTPError(int errorCode);

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
     * @brief Get formatted date (day + time) as a string
     * @param includeWeekday Include the weekday in the formatted string
     * @return Formatted date string
     */
    String getFormattedDate(bool includeWeekday = true);

    /**
     * @brief Get formatted time as a string
     * @return Formatted time string
     */
    String getFormattedTime();

    /**
     * @brief Get current hour (0-23)
     * @return Current hour
     */
    int getHour();

    /**
     * @brief Get current day of week (0=Sunday, 6=Saturday)
     * @return Day of week
     */
    int getDayOfWeek();

private:
    // Private helper methods still used internally
    time_t getCurrentTime();
    String getCurrentTimezone() const;
    String formatDate(const struct tm *t, bool includeWeekday = true);
    String formatTime(const struct tm *t);
    bool setupWithFallbackTimezone();
};

// Global instance for easy access
extern RTC rtc;
