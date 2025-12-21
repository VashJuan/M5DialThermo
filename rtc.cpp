/**
* WS1850S is the internal Real-Time-Clock in the M5Dial
* https://shop.m5stack.com/products/rfid-unit-2-ws1850s
* or uses NXP's PCF8563 RTC https://www.nxp.com/docs/en/data-sheet/PCF8563.pdf
* https://docs.m5stack.com/en/arduino/m5unified/rtc8563_class
*/

#include "rtc.hpp"

// Default configuration constants
static const char* DEFAULT_WIFI_SSID = "my_wifi_SSID";
static const char* DEFAULT_WIFI_PASSWORD = "my_password";
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
    M5Dial.Display.print("WiFi:");
    
    WiFi.begin(wifiConfig.ssid, wifiConfig.password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        Serial.print('.');
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\r\n WiFi Connected.");
        M5Dial.Display.print("Connected.");
        return true;
    } else {
        Serial.println("\r\n WiFi Connection Failed.");
        M5Dial.Display.print("Failed.");
        return false;
    }
}

// https://en.wikipedia.org/wiki/Network_Time_Protocol
bool RTC::synchronizeNTP() {
    Serial.println("Synchronizing with NTP...");
    
    configTzTime(ntpConfig.timezone, ntpConfig.server1, ntpConfig.server2, ntpConfig.server3);

#if SNTP_ENABLED
    int attempts = 0;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && attempts < 30) {
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
    M5Dial.Rtc.setDateTime(gmtime(&t));
    
    return true;
}

bool RTC::setup() {
    Serial.println("RTC setup start");
    
    if (!M5Dial.Rtc.isEnabled()) {
        Serial.println("RTC not found.");
        M5Dial.Display.println("RTC not found.");
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
        Serial.println("RTC not initialized");
        return;
    }

    Serial.println("RTC update start");
    delay(500);

    auto dt = M5Dial.Rtc.getDateTime();
    Serial.printf("RTC   UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
                  dt.date.year, dt.date.month, dt.date.date,
                  weekdays[dt.date.weekDay], dt.time.hours, dt.time.minutes,
                  dt.time.seconds);
    M5Dial.Display.setCursor(0, 0);
    M5Dial.Display.printf("RTC   UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d",
                          dt.date.year, dt.date.month, dt.date.date,
                          weekdays[dt.date.weekDay], dt.time.hours, dt.time.minutes,
                          dt.time.seconds);

    /// ESP32 internal timer
    auto t = time(nullptr);
    {
        auto tm = gmtime(&t); // for UTC.
        Serial.printf("ESP32 UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
                      tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                      weekdays[tm->tm_wday], tm->tm_hour, tm->tm_min, tm->tm_sec);
        M5Dial.Display.setCursor(0, 20);
        M5Dial.Display.printf("ESP32 UTC  :%04d/%02d/%02d (%s)  %02d:%02d:%02d",
                              tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                              weekdays[tm->tm_wday], tm->tm_hour, tm->tm_min,
                              tm->tm_sec);
    }

    {
        auto tm = localtime(&t); // for local timezone.
        Serial.printf("ESP32 %s:%04d/%02d/%02d (%s)  %02d:%02d:%02d\r\n",
                      ntpConfig.timezone, tm->tm_year + 1900, tm->tm_mon + 1,
                      tm->tm_mday, weekdays[tm->tm_wday], tm->tm_hour, tm->tm_min,
                      tm->tm_sec);
        M5Dial.Display.setCursor(0, 40);
        M5Dial.Display.printf("ESP32 %s:%04d/%02d/%02d (%s)  %02d:%02d:%02d",
                              ntpConfig.timezone, tm->tm_year + 1900, tm->tm_mon + 1,
                              tm->tm_mday, weekdays[tm->tm_wday], tm->tm_hour,
                              tm->tm_min, tm->tm_sec);
    }

    Serial.println("RTC update end");
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

auto RTC::getRTCDateTime() -> decltype(M5Dial.Rtc.getDateTime()) {
    return M5Dial.Rtc.getDateTime();
}

void RTC::setDateTime(const m5::rtc_datetime_t& dateTime) {
    M5Dial.Rtc.setDateTime(dateTime);
}

bool RTC::isRTCEnabled() {
    return M5Dial.Rtc.isEnabled();
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
    String formatted;
    if (includeWeekday) {
        formatted = String(weekdays[t->tm_wday]) + " ";
    }
    formatted += String(t->tm_year + 1900) + "/" + 
                String(t->tm_mon + 1) + "/" + 
                String(t->tm_mday) + " " +
                String(t->tm_hour) + ":" + 
                String(t->tm_min) + ":" + 
                String(t->tm_sec);
    return formatted;
}

bool RTC::isSystemInitialized() const {
    return isInitialized;
}

// Global instance for easy access
RTC rtc;