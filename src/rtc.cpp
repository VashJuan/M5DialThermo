/**
 * @file rtc.cpp
 * @brief Real-Time Clock Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "rtc.hpp"
#include "secrets.h"

// Global instance for easy access
RTC rtc;

// Default configuration constants
// WiFi credentials are now defined in secrets.h
static const char* DEFAULT_NTP_TIMEZONE = "UTC-8";
static const char* DEFAULT_NTP_SERVER1 = "0.pool.ntp.org";
static const char* DEFAULT_NTP_SERVER2 = "1.pool.ntp.org";
static const char* DEFAULT_NTP_SERVER3 = "2.pool.ntp.org";

// Static weekday strings
static constexpr const char *const weekdays[7] = {"Sun", "Mon", "Tue", "Wed",
                                                  "Thr", "Fri", "Sat"};

// Constructor with default configuration
RTC::RTC() : isInitialized(false) {
    wifiConfig.ssid = DEFAULT_WIFI_SSID;
    wifiConfig.password = DEFAULT_WIFI_PASSWORD;
    ntpConfig.timezone = DEFAULT_NTP_TIMEZONE;
    ntpConfig.server1 = DEFAULT_NTP_SERVER1;
    ntpConfig.server2 = DEFAULT_NTP_SERVER2;
    ntpConfig.server3 = DEFAULT_NTP_SERVER3;
}

// Constructor with custom configuration
RTC::RTC(const WiFiConfig& wifi, const NTPConfig& ntp) : 
    wifiConfig(wifi), ntpConfig(ntp), isInitialized(false) {
}

// Destructor
RTC::~RTC() {
    // Cleanup if needed
}

bool RTC::connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 500) {
        Serial.print('.');
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\r\n WiFi Connected.");
        return true;
    } else {
        Serial.println("\r\n WiFi Connection Failed.");
        return false;
    }
}

// https://en.wikipedia.org/wiki/Network_Time_Protocol
bool RTC::synchronizeNTP() {
    const int MAX_NTP_ATTEMPTS = 1000;
    Serial.println("Synchronizing with NTP...");
    
    configTzTime(ntpConfig.timezone, ntpConfig.server1, ntpConfig.server2, ntpConfig.server3);

#if SNTP_ENABLED
    int attempts = 0;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && attempts < MAX_NTP_ATTEMPTS) {
        Serial.print('.');
        delay(1000);
        attempts++;
    }
    
    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Serial.println("\r\n NTP Synchronization Failed (SNTP enabled).");
        return false;
    }
#else
    delay(1600);
    struct tm timeInfo;
    int attempts = 0;
    while (!getLocalTime(&timeInfo, 1000) && attempts < 10) {
        Serial.print('.');
        attempts++;
    }
    
    if (attempts >= 10) {
        Serial.println("\r\n NTP Synchronization Failed.");
        return false;
    }
#endif

    Serial.println("\r\n NTP Connected.");
    
    // Set RTC time
    time_t t = time(nullptr) + 1; // Advance one second.
    while (t > time(nullptr)); // Synchronization in seconds
    M5.Rtc.setDateTime(gmtime(&t));
    
    return true;
}

bool RTC::setup() {
    Serial.println("RTC setup start");
    
    if (!M5.Rtc.isEnabled()) {
        Serial.println("RTC not found.");
        return false;
    }

    Serial.println("RTC found.");
    
    // Connect to WiFi
    if (!connectToWiFi()) {
        return false;
    }
    
    // Synchronize with NTP
    if (!synchronizeNTP()) {
        return false;
    }
    
    isInitialized = true;
    Serial.println("RTC setup end");
    return true;
}

void RTC::update() {
    if (!isInitialized) {
        static unsigned long loopCounter = 0;
        if (!(loopCounter++ % 100)) {
            Serial.println("RTC not initialized");
        }
        return;
    }

    Serial.println("RTC update start");
    delay(50);

    auto dt = M5.Rtc.getDateTime();
    Serial.printf("RTC   UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
                  dt.date.year, dt.date.month, dt.date.date,
                  weekdays[dt.date.weekDay], dt.time.hours, dt.time.minutes,
                  dt.time.seconds);

    /// ESP32 internal timer
    auto t = time(nullptr);
    {
        auto tm = gmtime(&t); // for UTC.
        Serial.println("ESP32 UTC  :" + formatTime(tm));
    }

    {
        auto tm = localtime(&t); // for local timezone.
        Serial.println("ESP32 " + String(ntpConfig.timezone) + ":" + formatTime(tm));
    }

    Serial.println("RTC update end");
}

String RTC::getFormattedTime(bool includeWeekday) {
    time_t now = getCurrentTime();
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Time unavailable";
    }
    return formatTime(&timeinfo, includeWeekday);
}

time_t RTC::getCurrentTime() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return (time_t)0;
    }
    time(&now);
    return now;
}

void RTC::setDateTime(const m5::rtc_datetime_t& dateTime) {
    M5.Rtc.setDateTime(dateTime);
}

bool RTC::isRTCEnabled() {
    return M5.Rtc.isEnabled();
}

void RTC::setWiFiCredentials(const char* ssid, const char* password) {
    wifiConfig.ssid = ssid;
    wifiConfig.password = password;
}

void RTC::setNTPConfig(const char* timezone, const char* server1, 
                       const char* server2, const char* server3) {
    ntpConfig.timezone = timezone;
    ntpConfig.server1 = server1;
    ntpConfig.server2 = server2;
    ntpConfig.server3 = server3;
}

String RTC::formatTime(const struct tm* t, bool includeWeekday) {
    struct tm timeinfo;
    const struct tm* timePtr;

        timePtr = t;
    
    // Convert 24-hour to 12-hour format with AM/PM
    int hour12 = timePtr->tm_hour;
    String ampm = "AM";
    
    if (hour12 == 0) {
        hour12 = 12; // Midnight is 12 AM
    } else if (hour12 == 12) {
        ampm = "PM"; // Noon is 12 PM
    } else if (hour12 > 12) {
        hour12 -= 12; // Convert PM hours
        ampm = "PM";
    }
    // Hours 1-11 remain the same with AM
    
    String formatted;
    if (includeWeekday) {
        formatted = String(weekdays[timePtr->tm_wday]) + " ";
    }
    formatted += String(timePtr->tm_year + 1900) + "/" + 
                String(timePtr->tm_mon + 1) + "/" + 
                String(timePtr->tm_mday) + " " +
                String(hour12) + ":" + 
                String(timePtr->tm_min < 10 ? "0" : "") + String(timePtr->tm_min) + ":" + 
                String(timePtr->tm_sec < 10 ? "0" : "") + String(timePtr->tm_sec) + " " +
                ampm;
    return formatted;
}

bool RTC::isSystemInitialized() const {
    return isInitialized;
}


int RTC::getHour() {
    auto dt = M5.Rtc.getDateTime();
    return dt.time.hours;
}

int RTC::getDayOfWeek() {
    auto dt = M5.Rtc.getDateTime();
    // Convert M5 day of week (1=Monday) to standard (0=Sunday)
    return (dt.date.weekDay == 7) ? 0 : dt.date.weekDay;
}
