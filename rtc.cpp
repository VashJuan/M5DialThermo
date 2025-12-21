/**
 * @file rtc.cpp
 * @brief Real-Time Clock Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "rtc.hpp"

void setup() {
  m5Dial.begin();
  Serial.println("RTC Test");

  if (!m5Dial.Rtc.isEnabled()) {
    Serial.println("RTC not found");
    while (true) {
      delay(500);
    }
  }
  Serial.println("RTC found");

  m5Dial.Display.print("WiFi: ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    m5Dial.Display.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");

  configTzTime(NTP_TIMEZONE, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  m5Dial.Display.print("NTP: ");
#if SNTP_ENABLED
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    m5Dial.Display.print(".");
    delay(500);
  }
#else
  struct tm timeInfo;
  while (!getLocalTime(&timeInfo, 1000)) {
    m5Dial.Display.print(".");
    delay(500);
  }
#endif
  Serial.println("\nNTP connected");

  time_t t = time(nullptr) + 1;  // Advance one second
  while (t > time(nullptr))
    ;  // Synchronization in seconds
  m5Dial.Rtc.setDateTime(gmtime(&t));

  delay(1000);
  //m5Dial.Display.clear();
  Serial.println("RTC Test");
}

void loop() {
  auto dt = m5Dial.Rtc.getDateTime();
  Serial.printf("RTC   UTC  :");
  Serial.printf("%04d/%02d/%02d(%s)", dt.date.year, dt.date.month, dt.date.date, wd[dt.date.weekDay]);
  Serial.printf("%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);

  // ESP32 internal timer
  auto t = time(nullptr);

  {
    auto tm = gmtime(&t);  // for UTC
    Serial.printf("ESP32 UTC  :");
    Serial.printf("%04d/%02d/%02d(%s)", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, wd[tm->tm_wday]);
    Serial.printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  }

  {
    auto tm = localtime(&t);  // for local timezone
    Serial.printf("ESP32 %s:", NTP_TIMEZONE);
    Serial.printf("%04d/%02d/%02d(%s)", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, wd[tm->tm_wday]);
    Serial.printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  }

  delay(500);
}

// Global instance for easy access
RTC rtc;