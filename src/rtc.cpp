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
static const char* DEFAULT_NTP_SERVER1 = "time.nist.gov";
static const char* DEFAULT_NTP_SERVER2 = "pool.ntp.org";
static const char* DEFAULT_NTP_SERVER3 = "0.pool.ntp.org";

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
    
    // Disconnect any previous connections
    WiFi.disconnect(true);
    delay(100);
    
    // Configure WiFi with improved settings
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false); // Disable WiFi sleep for better reliability
    
    // Configure DNS servers for better connectivity
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, IPAddress(8, 8, 8, 8), IPAddress(1, 1, 1, 1));
    
    Serial.printf("Connecting to SSID: %s\n", wifiConfig.ssid);
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    
    // Reduced timeout to respect watchdog timer (10 seconds max)
    int maxAttempts = 40; // 40 * 250ms = 10 seconds
    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED && maxAttempts > 0) {
        Serial.print('.');
        delay(200); // Reduced delay
        yield(); // Feed watchdog
        maxAttempts--;
        
        // Additional safety check for total time
        if (millis() - startTime > 10000) {
            Serial.println("\nWiFi connection timeout (10s)");
            break;
        }
        
        // Feed watchdog more frequently
        if (maxAttempts % 5 == 0) {
            yield();
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\r\nWiFi Connected.");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("DNS: %s, %s\n", WiFi.dnsIP(0).toString().c_str(), WiFi.dnsIP(1).toString().c_str());
        return true;
    } else {
        Serial.printf("\r\nWiFi Connection Failed. Status: %d\n", WiFi.status());
        Serial.println("Possible causes:");
        Serial.println("- Incorrect SSID/password");
        Serial.println("- WiFi network out of range");
        Serial.println("- Network congestion");
        Serial.println("- Router/AP issues");
        return false;
    }
}

// https://en.wikipedia.org/wiki/Network_Time_Protocol
bool RTC::synchronizeNTP() {
    Serial.println("Synchronizing with NTP...");
    
    // Test DNS connectivity first
    if (!testDNSConnectivity()) {
        Serial.println("DNS connectivity test failed");
        return false;
    }
    
    Serial.printf("Using NTP servers: %s, %s, %s\n", ntpConfig.server1, ntpConfig.server2, ntpConfig.server3);
    Serial.printf("Timezone: %s\n", ntpConfig.timezone);
    
    configTzTime(ntpConfig.timezone, ntpConfig.server1, ntpConfig.server2, ntpConfig.server3);
    
    // Wait for initial configuration
    delay(500); // Reduced delay
    yield(); // Feed watchdog

#if SNTP_ENABLED
    Serial.println("Using SNTP sync status method");
    unsigned long startTime = millis();
    int maxAttempts = 30; // 30 * 250ms = 7.5 seconds max
    
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && maxAttempts > 0) {
        Serial.print('.');
        delay(200); // Reduced delay
        yield(); // Feed watchdog
        maxAttempts--;
        
        // Safety timeout
        if (millis() - startTime > 7500) {
            Serial.println("\nSNTP timeout (7.5s)");
            break;
        }
        
        // Extra watchdog feeding
        if (maxAttempts % 3 == 0) {
            yield();
        }
    }
    
    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Serial.printf("\r\nNTP Synchronization Failed (SNTP enabled). Status: %d\n", sntp_get_sync_status());
        return tryAlternativeNTPSync();
    }
#else
    Serial.println("Using getLocalTime method (SNTP not available)");
    return tryAlternativeNTPSync();
#endif

    Serial.println("\r\nNTP Connected via SNTP.");
    
    // Verify and set RTC time
    return setRTCFromNTP();
}

bool RTC::tryAlternativeNTPSync() {
    Serial.println("Trying alternative NTP sync method...");
    
    unsigned long startTime = millis();
    struct tm timeInfo;
    int maxAttempts = 15; // 15 * 400ms = 6 seconds max
    
    while (maxAttempts > 0) {
        yield(); // Feed watchdog before each attempt
        if (getLocalTime(&timeInfo, 400)) {
            // Check if we got a reasonable time (after 2020)
            if (timeInfo.tm_year + 1900 >= 2020) {
                Serial.println("\r\nNTP Connected via getLocalTime.");
                return setRTCFromNTP();
            }
        }
        
        Serial.print('.');
        yield(); // Feed watchdog
        maxAttempts--;
        
        // Safety timeout
        if (millis() - startTime > 6000) {
            Serial.println("\nAlternative NTP timeout (6s)");
            break;
        }
    }
    
    Serial.println("\r\nNTP Synchronization Failed (both methods).");
    Serial.println("Possible causes:");
    Serial.println("- Firewall blocking NTP (port 123)");
    Serial.println("- NTP servers unreachable");
    Serial.println("- Network connectivity issues");
    Serial.println("- DNS resolution problems");
    
    return false;
}

bool RTC::setRTCFromNTP() {
    // Set RTC time with proper conversion
    time_t now = time(nullptr);
    struct tm *timeinfo = gmtime(&now);
    
    if (timeinfo && timeinfo->tm_year + 1900 >= 2020) {
        // Convert tm struct to M5 RTC datetime format
        m5::rtc_datetime_t dt;
        dt.date.year = timeinfo->tm_year + 1900;
        dt.date.month = timeinfo->tm_mon + 1;
        dt.date.date = timeinfo->tm_mday;
        dt.date.weekDay = timeinfo->tm_wday;
        dt.time.hours = timeinfo->tm_hour;
        dt.time.minutes = timeinfo->tm_min;
        dt.time.seconds = timeinfo->tm_sec;
        
        M5.Rtc.setDateTime(dt);
        
        Serial.printf("RTC hardware updated: %04d/%02d/%02d (%s) %02d:%02d:%02d UTC\n",
                      dt.date.year, dt.date.month, dt.date.date,
                      weekdays[dt.date.weekDay], dt.time.hours, dt.time.minutes, dt.time.seconds);
        return true;
    } else {
        Serial.println("Warning: Failed to get valid time for RTC update");
        return false;
    }
}

bool RTC::testDNSConnectivity() {
    Serial.println("Testing DNS connectivity...");
    
    // Try to resolve a known domain
    IPAddress result;
    int dnsResult = WiFi.hostByName("google.com", result);
    
    if (dnsResult == 1) {
        Serial.printf("DNS test successful: google.com -> %s\n", result.toString().c_str());
        return true;
    } else {
        Serial.printf("DNS test failed: error %d\n", dnsResult);
        
        // Try with the NTP server directly
        dnsResult = WiFi.hostByName(ntpConfig.server1, result);
        if (dnsResult == 1) {
            Serial.printf("NTP server DNS resolution successful: %s -> %s\n", ntpConfig.server1, result.toString().c_str());
            return true;
        } else {
            Serial.printf("NTP server DNS resolution failed: %s (error %d)\n", ntpConfig.server1, dnsResult);
            return false;
        }
    }
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
        WiFi.disconnect(); // Clean up
        return setupWithFallbackTimezone();
    }
    
    // Skip automatic timezone detection for now to avoid blocking
    Serial.println("Skipping automatic timezone detection to avoid watchdog timeout");
    Serial.println("Using configured default timezone");
    yield(); // Feed watchdog;
    
    // Try NTP sync with robust error handling
    bool ntpSuccess = false;
    unsigned long syncStartTime = millis();
    
    try {
        yield(); // Feed watchdog before sync
        ntpSuccess = synchronizeNTP();
        yield(); // Feed watchdog after sync
    } catch (...) {
        Serial.println("Exception during NTP sync");
        ntpSuccess = false;
    }
    
    // Check for overall timeout (reduced to 10 seconds)
    if (millis() - syncStartTime > 10000) {
        Serial.println("NTP sync overall timeout (10s)");
        ntpSuccess = false;
    }
    
    if (!ntpSuccess) {
        Serial.println("NTP sync failed, using fallback timezone...");
        WiFi.disconnect(); // Clean up WiFi connection
        WiFi.mode(WIFI_OFF); // Completely disable WiFi
        yield(); // Feed watchdog instead of delay
        return setupWithFallbackTimezone();
    }
    
    // SUCCESS: Disable WiFi after successful sync to prevent background operations
    Serial.println("NTP sync successful - disabling WiFi to prevent background operations");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    yield(); // Feed watchdog
    
    isInitialized = true;
    Serial.println("RTC setup complete - WiFi disabled");
    return true;
}

void RTC::update() {
    if (!isInitialized) {
        static unsigned long loopCounter = 200;
        if ((loopCounter--) % 100) {
            Serial.println("RTC not initialized");
        }
        return;
    }

    yield(); // Feed watchdog at start
    Serial.println("\nRTC update start");
    delay(30); // Reduced delay
    yield(); // Feed watchdog

    auto dt = M5.Rtc.getDateTime();
    Serial.printf("RTC   UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
                  dt.date.year, dt.date.month, dt.date.date,
                  weekdays[dt.date.weekDay], dt.time.hours, dt.time.minutes,
                  dt.time.seconds);
    
    // Check if RTC time looks valid (not year 2000)
    if (dt.date.year < 2020) {
        Serial.println("Warning: RTC hardware appears to have invalid date (year < 2020)");
        Serial.println("This may indicate RTC hardware synchronization failed");
        Serial.println("Note: Automatic resync disabled to prevent WiFi operations");
        
        // Force RTC sync from ESP32 time without WiFi
        time_t esp32Time = time(nullptr);
        if (esp32Time > 1640000000) { // Check if ESP32 time looks reasonable (after 2022)
            static unsigned long lastResyncAttempt = 0;
            if (millis() - lastResyncAttempt > 60000) { // Don't spam resync attempts
                lastResyncAttempt = millis();
                Serial.println("Attempting RTC sync from ESP32 internal time (no WiFi)");
                forceRTCSync();
            }
        }
    }

    /// ESP32 internal timer
    auto t = time(nullptr);
    {
        auto tm = gmtime(&t); // for UTC.
        Serial.println("ESP32 UTC  :" + formatTime(tm));
    }

    {
        auto tm = localtime(&t); // for local timezone.
        String currentTimezone = fallbackTimezone.length() > 0 ? fallbackTimezone : String(ntpConfig.timezone);
        
        // Show a more user-friendly timezone label for common zones
        String displayTimezone = currentTimezone;
        if (currentTimezone.startsWith("PST8PDT")) {
            displayTimezone = "PST/PDT";
        } else if (currentTimezone.startsWith("EST5EDT")) {
            displayTimezone = "EST/EDT";
        } else if (currentTimezone.startsWith("MST7MDT")) {
            displayTimezone = "MST/MDT";
        } else if (currentTimezone.startsWith("CST6CDT")) {
            displayTimezone = "CST/CDT";
        } else if (currentTimezone == "UTC-8") {
            displayTimezone = "PST";
        } else if (currentTimezone == "UTC-5") {
            displayTimezone = "EST";
        } else if (currentTimezone == "UTC-7") {
            displayTimezone = "MST";
        } else if (currentTimezone == "UTC-6") {
            displayTimezone = "CST";
        }
        
        Serial.println("ESP32 Local " + displayTimezone + ":" + formatTime(tm));
    }

    Serial.println("RTC update end\n");
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
    
    // Debug: Print timezone and raw time info
    // Serial.printf("Debug: Using timezone: %s\n", currentTimezone.c_str());
    
    // Return properly formatted local time without confusing timezone suffix
    String timeStr = formatTime(&timeinfo, includeWeekday);
    return timeStr;
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

bool RTC::forceRTCSync() {
    Serial.println("Forcing RTC hardware synchronization...");
    
    // Get current ESP32 time
    time_t now = time(nullptr);
    if (now == 0) {
        Serial.println("Error: No valid time available for RTC sync");
        return false;
    }
    
    struct tm *timeinfo = gmtime(&now);
    if (!timeinfo) {
        Serial.println("Error: Failed to convert time for RTC sync");
        return false;
    }
    
    // Convert tm struct to M5 RTC datetime format (UTC)
    m5::rtc_datetime_t dt;
    dt.date.year = timeinfo->tm_year + 1900;
    dt.date.month = timeinfo->tm_mon + 1;
    dt.date.date = timeinfo->tm_mday;
    dt.date.weekDay = timeinfo->tm_wday;
    dt.time.hours = timeinfo->tm_hour;
    dt.time.minutes = timeinfo->tm_min;
    dt.time.seconds = timeinfo->tm_sec;
    
    // Set the RTC hardware
    M5.Rtc.setDateTime(dt);
    
    Serial.printf("RTC hardware forcibly updated to: %04d/%02d/%02d (%s) %02d:%02d:%02d UTC\n",
                  dt.date.year, dt.date.month, dt.date.date,
                  weekdays[dt.date.weekDay], dt.time.hours, dt.time.minutes, dt.time.seconds);
    
    // Verify the update worked
    delay(100);
    auto verifyDt = M5.Rtc.getDateTime();
    if (verifyDt.date.year >= 2020) {
        Serial.println("RTC hardware sync verification: SUCCESS");
        return true;
    } else {
        Serial.println("RTC hardware sync verification: FAILED");
        return false;
    }
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
    //Serial.printf("File size: %d bytes\n", file.size());
    
    String line;
    bool timezoneSet = false;
    int lineNum = 0;
    
    while (file.available() && !timezoneSet) {
        line = file.readStringUntil('\n');
        line.trim();
        lineNum++;
        
        // Serial.printf("Line %d: '%s'\n", lineNum, line.c_str());
        
        // Skip comments and empty lines
        if (line.length() == 0 || line.startsWith("#")) {
            continue;
        }
        
        // Parse fallback timezone
        if (line.startsWith("FallbackTimezone,")) {
            // Serial.println("Found FallbackTimezone line!");
            int commaIndex = line.indexOf(',');
            if (commaIndex != -1) {
                String fullTimezone = line.substring(commaIndex + 1);
                fullTimezone.trim();
                
                // Handle complex timezone formats (e.g., PST8PDT,M3.2.0,M11.1.0)
                // Take the entire string as ESP32 supports complex timezone formats
                fallbackTimezone = fullTimezone;
                timezoneSet = true;
                Serial.printf("Loaded fallback timezone: '%s'\n", fallbackTimezone.c_str());
                
                // Also log the simplified explanation for debugging
                if (fallbackTimezone.startsWith("PST8PDT")) {
                    Serial.println("  -> Pacific Standard Time with Daylight Saving Time");
                } else if (fallbackTimezone.startsWith("EST5EDT")) {
                    Serial.println("  -> Eastern Standard Time with Daylight Saving Time");
                } else if (fallbackTimezone.startsWith("MST7MDT")) {
                    Serial.println("  -> Mountain Standard Time with Daylight Saving Time");
                } else if (fallbackTimezone.startsWith("CST6CDT")) {
                    Serial.println("  -> Central Standard Time with Daylight Saving Time");
                } else if (fallbackTimezone.startsWith("UTC")) {
                    Serial.println("  -> Coordinated Universal Time");
                }
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
    delay(50); // Reduced delay
    yield(); // Feed watchdog
    
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
    
    // Ensure WiFi is completely disabled to prevent background operations
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    yield(); // Feed watchdog
    
    isInitialized = true;
    Serial.println("Fallback timezone setup complete - WiFi disabled");
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
    http.setTimeout(15000); // Increased to 15 second timeout for better reliability
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
        String payload = http.getString();
        http.end(); // Close connection immediately
        
        // Parse JSON response with smaller buffer
        JsonDocument doc;
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
