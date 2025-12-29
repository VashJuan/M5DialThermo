/**
 * @file rtc.cpp
 * @brief Real-Time Clock Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "rtc.hpp"
#include "secrets.h"
#include "SPIFFS.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"

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
    fallbackTimezone = DEFAULT_NTP_TIMEZONE;
}

// Constructor with custom configuration
RTC::RTC(const WiFiConfig& wifi, const NTPConfig& ntp) : 
    wifiConfig(wifi), ntpConfig(ntp), isInitialized(false) {
    fallbackTimezone = ntp.timezone;
}

// Destructor
RTC::~RTC() {
    // Cleanup if needed
}

bool RTC::connectToWiFi() {
    Serial.println("Connecting to WiFi...");
    
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 100) { // Reduced from 500
        Serial.print('.');
        delay(100); // Reduced from 500ms
        yield(); // Feed watchdog
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
    const int MAX_NTP_ATTEMPTS = 50; // Reduced from 1000
    Serial.println("Synchronizing with NTP...");
    
    configTzTime(ntpConfig.timezone, ntpConfig.server1, ntpConfig.server2, ntpConfig.server3);

#if SNTP_ENABLED
    int attempts = 0;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && attempts < MAX_NTP_ATTEMPTS) {
        Serial.print('.');
        delay(200); // Reduced from 1000ms
        yield(); // Feed watchdog
        attempts++;
    }
    
    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Serial.println("\r\n NTP Synchronization Failed (SNTP enabled).");
        return false;
    }
#else
    delay(500); // Reduced from 1600ms
    yield(); // Feed watchdog
    struct tm timeInfo;
    int attempts = 0;
    while (!getLocalTime(&timeInfo, 500) && attempts < 5) { // Reduced attempts from 10
        Serial.print('.');
        yield(); // Feed watchdog
        attempts++;
    }
    
    if (attempts >= 5) {
        Serial.println("\r\n NTP Synchronization Failed.");
        return false;
    }
#endif

    Serial.println("\r\n NTP Connected.");
    
    // Set RTC time
    time_t t = time(nullptr) + 1; // Advance one second.
    while (t > time(nullptr)) {
        yield(); // Feed watchdog during wait
    }
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
    
    // Connect to WiFi with shorter timeout
    if (!connectToWiFi()) {
        Serial.println("WiFi connection failed, using fallback timezone...");
        return setupWithFallbackTimezone();
    }
    
    // Skip automatic timezone detection for now to avoid blocking
    Serial.println("Skipping automatic timezone detection to avoid watchdog timeout");
    Serial.println("Using configured default timezone");
    
    // Configure timezone (use default)
    configTzTime(ntpConfig.timezone, ntpConfig.server1, ntpConfig.server2, ntpConfig.server3);
    delay(100);
    yield(); // Feed watchdog
    
    // Try NTP sync with shorter timeout
    if (!synchronizeNTP()) {
        Serial.println("NTP sync failed, using fallback timezone...");
        WiFi.disconnect();
        return setupWithFallbackTimezone();
    }
    
    isInitialized = true;
    Serial.println("RTC setup complete");
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
        String currentTimezone = fallbackTimezone.length() > 0 ? fallbackTimezone : String(ntpConfig.timezone);
        Serial.println("ESP32 " + currentTimezone + ":" + formatTime(tm));
    }

    Serial.println("RTC update end");
}

String RTC::getFormattedTime(bool includeWeekday) {
    if (!isInitialized) {
        return "RTC not initialized";
    }
    
    // Ensure timezone is configured
    String currentTimezone = getCurrentTimezone();
    if (currentTimezone.length() == 0) {
        Serial.println("Warning: No timezone configured, using UTC");
        return "Time unavailable (no timezone)";
    }
    
    time_t now = getCurrentTime();
    if (now == 0) {
        return "Time unavailable";
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.printf("Warning: getLocalTime() failed with timezone: %s\n", currentTimezone.c_str());
        return "Time unavailable";
    }
    
    // Add timezone suffix to indicate what timezone we're showing
    String timeStr = formatTime(&timeinfo, includeWeekday);
    return timeStr + " (" + currentTimezone + ")";
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

bool RTC::loadFallbackTimezone() {
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Warning: Failed to mount SPIFFS filesystem for timezone fallback");
        return false;
    }
    
    // Try to open from SPIFFS
    File file = SPIFFS.open("/temps.csv", "r");
    if (!file) {
        // Try without leading slash
        file = SPIFFS.open("temps.csv", "r");
    }
    
    if (!file) {
        Serial.println("Warning: Could not open temps.csv for timezone fallback");
        Serial.println("Available files in SPIFFS:");
        File root = SPIFFS.open("/");
        File foundFile = root.openNextFile();
        while (foundFile) {
            Serial.printf("  - %s\n", foundFile.name());
            foundFile = root.openNextFile();
        }
        return false;
    }

    Serial.println("Loading fallback timezone from temps.csv");
    Serial.printf("File size: %d bytes\n", file.size());
    
    String line;
    bool timezoneSet = false;
    int lineNum = 0;
    
    while (file.available() && !timezoneSet) {
        line = file.readStringUntil('\n');
        line.trim();
        lineNum++;
        
        Serial.printf("Line %d: '%s'\n", lineNum, line.c_str());
        
        // Skip comments and empty lines
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }
        
        // Parse fallback timezone
        if (line.startsWith("FallbackTimezone,")) {
            Serial.println("Found FallbackTimezone line!");
            int commaIndex = line.indexOf(',');
            if (commaIndex != -1) {
                fallbackTimezone = line.substring(commaIndex + 1);
                fallbackTimezone.trim();
                timezoneSet = true;
                Serial.printf("Loaded fallback timezone: '%s'\n", fallbackTimezone.c_str());
            } else {
                Serial.println("Error: No comma found in FallbackTimezone line");
            }
        }
    }
    
    file.close();
    
    if (!timezoneSet) {
        Serial.printf("Warning: Fallback timezone not found in CSV, using default: %s\n", DEFAULT_NTP_TIMEZONE);
        fallbackTimezone = DEFAULT_NTP_TIMEZONE;
    }
    
    return timezoneSet;
}

bool RTC::setupWithFallbackTimezone() {
    Serial.println("Setting up RTC with fallback timezone (no NTP sync)");
    
    // Load fallback timezone from CSV
    if (!loadFallbackTimezone()) {
        Serial.println("Using hardcoded fallback timezone");
    }
    
    // Configure timezone without NTP servers
    configTzTime(fallbackTimezone.c_str(), nullptr, nullptr, nullptr);
    
    // Wait a moment for timezone to take effect
    delay(100);
    
    // Validate timezone configuration
    struct tm timeinfo;
    time_t testTime = time(nullptr);
    if (getLocalTime(&timeinfo)) {
        Serial.printf("Timezone configured successfully: %s\n", fallbackTimezone.c_str());
        Serial.printf("Local time: %s\n", formatTime(&timeinfo).c_str());
    } else {
        Serial.printf("Warning: Timezone configuration may have failed: %s\n", fallbackTimezone.c_str());
    }
    
    Serial.println("Note: Time will not be synchronized with NTP servers");
    Serial.println("Manual time adjustment may be required for accuracy");
    
    isInitialized = true;
    return true;
}

String RTC::getFallbackTimezone() const {
    return fallbackTimezone;
}

String RTC::getCurrentTimezone() const {
    // Return the currently active timezone
    if (fallbackTimezone.length() > 0) {
        return fallbackTimezone;
    }
    return String(ntpConfig.timezone);
}

bool RTC::updateFallbackTimezone(const String& newTimezone) {
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Error: Failed to mount SPIFFS filesystem for timezone update");
        return false;
    }
    
    // Read the entire file content
    File file = SPIFFS.open("/temps.csv", "r");
    if (!file) {
        file = SPIFFS.open("temps.csv", "r");
    }
    
    if (!file) {
        Serial.println("Error: Could not open temps.csv for timezone update");
        return false;
    }
    
    String content = "";
    bool timezoneUpdated = false;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        
        if (line.startsWith("FallbackTimezone,")) {
            content += "FallbackTimezone," + newTimezone + "\n";
            timezoneUpdated = true;
        } else {
            content += line + "\n";
        }
    }
    file.close();
    
    // If timezone line wasn't found, add it after BaseTemperature
    if (!timezoneUpdated) {
        // Insert the timezone line after BaseTemperature
        int basePos = content.indexOf("BaseTemperature,");
        if (basePos != -1) {
            int nextLine = content.indexOf('\n', basePos);
            if (nextLine != -1) {
                String newContent = content.substring(0, nextLine + 1);
                newContent += "\n# Fallback timezone for when NTP/WiFi is not available\n";
                newContent += "FallbackTimezone," + newTimezone + "\n";
                newContent += content.substring(nextLine + 1);
                content = newContent;
                timezoneUpdated = true;
            }
        }
    }
    
    if (!timezoneUpdated) {
        Serial.println("Error: Could not locate appropriate position to update timezone");
        return false;
    }
    
    // Write the updated content back
    file = SPIFFS.open("/temps.csv", "w");
    if (!file) {
        file = SPIFFS.open("temps.csv", "w");
    }
    
    if (!file) {
        Serial.println("Error: Could not open temps.csv for writing timezone update");
        return false;
    }
    
    file.print(content);
    file.close();
    
    // Update the internal fallback timezone
    fallbackTimezone = newTimezone;
    
    Serial.printf("Successfully updated fallback timezone to: %s\n", newTimezone.c_str());
    return true;
}

bool RTC::detectTimezoneFromLocation() {
    Serial.println("Attempting automatic timezone detection...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected for timezone detection");
        return false;
    }
    
    HTTPClient http;
    http.begin("http://worldtimeapi.org/api/ip");
    http.setTimeout(5000); // Reduced to 5 second timeout
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String payload = http.getString();
        http.end(); // Close connection immediately
        
        // Parse JSON response with smaller buffer
        DynamicJsonDocument doc(512); // Reduced buffer size
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.printf("JSON parsing failed: %s\n", error.c_str());
            return false;
        }
        
        // Extract timezone from response
        const char* detectedTimezone = doc["timezone"];
        const char* utcOffset = doc["utc_offset"];
        
        if (detectedTimezone && utcOffset) {
            // Convert to ESP32 timezone format
            String espTimezone = convertToESP32Timezone(String(utcOffset), String(detectedTimezone));
            
            if (espTimezone.length() > 0) {
                Serial.printf("Detected timezone: %s (UTC%s)\n", detectedTimezone, utcOffset);
                Serial.printf("Using ESP32 timezone: %s\n", espTimezone.c_str());
                
                // Update the NTP configuration with detected timezone
                ntpConfig.timezone = espTimezone.c_str();
                
                return true; // Don't configure timezone here, do it in setup
            } else {
                Serial.printf("Failed to convert timezone format: %s -> ESP32\n", utcOffset);
            }
        } else {
            Serial.println("Invalid timezone data received from API");
        }
    } else {
        Serial.printf("HTTP request failed: %d\n", httpResponseCode);
    }
    
    http.end();
    return false;
}

String RTC::convertToESP32Timezone(const String& utcOffset, const String& timezoneName) {
    // Convert UTC offset (e.g., "+08:00", "-05:00") to ESP32 format (e.g., "UTC-8", "UTC+5")
    if (utcOffset.length() < 6) {
        return "";
    }
    
    char sign = utcOffset.charAt(0);
    int hours = utcOffset.substring(1, 3).toInt();
    int minutes = utcOffset.substring(4, 6).toInt();
    
    // ESP32 timezone format uses opposite sign convention
    String espSign = (sign == '+') ? "-" : "+";
    
    // Handle fractional hours (if minutes != 0)
    if (minutes == 0) {
        return "UTC" + espSign + String(hours);
    } else {
        float totalHours = hours + (minutes / 60.0);
        return "UTC" + espSign + String(totalHours, 1);
    }
}
