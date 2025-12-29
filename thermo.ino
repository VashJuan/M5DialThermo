/**
 * @file thermo.ino
 * @url https://github.com/vashjuan/M5Dial_thermo
 * @author John Cornelison (john@vashonSoftware.com)
 * @brief M5Dial thermo
 * @version 2.0.0
 * @date 2025-12-18
 *
 *
/**
 * @Hardware:  M5Dial
 *               (https://m5stack.com/products/m5dial)
 *               which uses the M5StampS3 as the controller
 *               (https://shop.m5stack.com/products/m5stamp-esp32s3-module)
 *               https://docs.m5stack.com/en/arduino/m5stamps3/program
 *             + Adafruit MCP9808 Precision I2C Temperature Sensor
 *               (http://www.adafruit.com/products/1782)
 *             + Grove-Wio-E5 Wireless Module - STM32WLE5JC, ARM Cortex-M4 and SX126x; EU868 & US915LoRaWAN
 *               (https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html)
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * Adafruit_MCP9808: https://github.com/adafruit/Adafruit_MCP9808_Library
 *
 * M5StampS3A, is an update to the M5Stamp S3 ESP32-S3 module introduced in 2023 with
 * an optimized antenna design, lower power consumption, a larger user button, and
 * a different logic for the RGB LED control.
 * - https://www.cnx-software.com/2025/07/11/m5stack-stamp-s3a-wifi-and-ble-iot-module-benefits-from-optimized-antenna-design-lower-power-consumption/
 **/

// Core M5Dial include must be first
#include <M5Dial.h>

// #include <Arduino.h>
#include "encoder.hpp"
#include "rtc.hpp"
#include "temp_sensor.hpp"
#include "stove.hpp"
#include "display.hpp"
#include "display.hpp"

/**
 * per https://docs.m5stack.com/en/core/M5Dial#pinmap:
 * https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/684/S007_PinMap_01.jpg
 *  White: M5Dial pins G15 SCL (serial clock) (for Port A) & G1 (for Port B)
 *  Yellow: M5Dial pins G13 SDA (serial data) (for Port A) & G2 (for Port B)
 *  Red: 5 volts
 *  Black: ground
 */
const int portA[] = {15, 13}; // G15 = SCL, G13 = SDA
const int portB[] = {1, 2};   // G1 = SCL, G2 = SDA

// MCP9808 uses I2C communication

int updateTime()
{
    rtc.update();
    display.showText(TIME, rtc.getFormattedTime());
    return rtc.getDayOfWeek() * 24 + rtc.getHour(); // hour of the week
}

float updateTemperature()
{
    // Read temperature sensor
    float temperature = tempSensor.readTemperatureFahrenheit();

    if (!tempSensor.isValidReading(temperature))
    {
        Serial.println("Invalid temperature reading");
        display.showText(STATUS, "Temperature Sensor Error", COLOR_RED);
        return 999.0;
    }

    // Serial.printf("Temperature: %.2f°F\n", temperature);
    display.showText(TEMP, String(temperature, 1) + " F");

    return temperature;
}

bool updateStove(float temperature, int hourOfWeek)
{
    // Signal stove to turn on/off based on the current temperature
    String stoveStatus = stove.update(temperature, hourOfWeek);
    if (stoveStatus.length() > 0)
    {
        Serial.println("Stove status: " + stoveStatus);
        display.showText(STOVE, "Stove: " + stoveStatus);
    }
    return stoveStatus == "ON";
}

void setup()
{
    Serial.begin(9600);
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    display.setup();
    display.showSplashScreen();

    Serial.print("Setting up encoder...");
    display.showText(TIME, "Setting up (encoder) dial...");
    delay(500);
    encoder.setup();

    Serial.println(" and RTC...");
    Serial.println("=========Display width returns: " + String(display.getWidth()));

    display.showText(TIME, "Setting up real time clock...");
    delay(500);
    rtc.setup();

    // Initialize temperature sensor
    if (!tempSensor.setup())
    {
        Serial.println("Failed to initialize temperature sensor!");
        display.showText(STATUS, "Temp Sensor Init Failed.", COLOR_RED);
        // Don't block - continue with other setup
    }
    else
    {
        Serial.printf("Temperature sensor initialized successfully at 0x%02X\n", tempSensor.getI2CAddress());
        Serial.printf("Current resolution: %s\n", tempSensor.getResolutionString());
    }

    String now = rtc.getFormattedTime();
    Serial.println("Setup done at " + now);
    display.showText(TIME, now);
}

static uint32_t lastActivityTime = millis();
bool recentActivity = false;
int activityTimeout = 3000; // 5 seconds

// Manual stove control state
static bool manualStoveOverride = false;
const float SAFETY_MAX_TEMP = 82.0; // Safety maximum temperature in °F

void noteActivity()
{
    recentActivity = true;
    lastActivityTime = millis();
}

int sleepShort = 1; // 1 second
int sleepLong = 10; // sleep-in if no recent activity
int loopCounter = 0;
// bool touch_wakeup = true; // Whether to enable touch screen wake-up

void loop()
{
    M5Dial.update();
    // encoder.update();

    // Read current values first so they're available for all handlers
    static int hourOfWeek = updateTime();
    static float curTemp = updateTemperature();

    if (encoder.hasPositionChanged())
    {
        long position = encoder.getPosition();
        Serial.printf("Encoder position: %ld\n", position);
        noteActivity();
    }

    if (M5Dial.BtnA.wasPressed())
    {
        M5Dial.Speaker.tone(8000, 20);
        delay(250);
        // M5Dial.Speaker.mute();

        // Toggle manual stove override with safety check
        if (!manualStoveOverride)
        {
            // Turning ON: check safety temperature limit
            if (curTemp <= SAFETY_MAX_TEMP)
            {
                manualStoveOverride = true;
                stove.forceState(true);
                Serial.println("Button A pressed: Manual stove override ON");
            }
            else
            {
                Serial.printf("Safety: Cannot turn on stove - temperature %.1f°F exceeds safety limit of %.1f°F\n", curTemp, SAFETY_MAX_TEMP);
                // Give audio feedback for safety override
                M5Dial.Speaker.tone(4000, 100);
                delay(100);
                M5Dial.Speaker.tone(4000, 100);
            }
        }
        else
        {
            // Turning OFF
            manualStoveOverride = false;
            stove.forceState(false);
            Serial.println("Button A pressed: Manual stove override OFF");
        }
        noteActivity();
    }

    if (M5Dial.BtnA.wasReleased())
    {
        M5Dial.Speaker.tone(12000, 20);
        delay(500);
        noteActivity();
    }

    bool stoveOn = updateStove(curTemp, hourOfWeek);

    // Display stove status including manual override
    if (manualStoveOverride)
    {
        display.showText(STOVE, "Stove: MANUAL ON");
    }
    else if (stoveOn)
    {
        display.showText(STOVE, "Stove: AUTO ON");
    }
    else
    {
        display.showText(STOVE, "Stove: OFF");
        // https://deepwiki.com/m5stack/M5Unified/4.3-sleep-modes-and-power-off#wakeup-pin-configuration
        // https://docs.m5stack.com/en/arduino/m5unified/power_class#lightsleep
        // if (!(loopCounter++ % 10))
        {
            Serial.println(String(loopCounter) + ") Sleeping for " + String((millis() - lastActivityTime > activityTimeout) ? sleepLong : sleepShort) + " seconds...");
        }

        tempSensor.shutdown();
        delay(((millis() - lastActivityTime > activityTimeout) ? sleepLong : sleepShort) * 1000);
        // This blacks out the display, so not what we want...
        // M5.Power.lightSleep(((millis() - lastActivityTime > activityTimeout) ? sleepLong : sleepShort) * 1000000ULL, touch_wakeup);
        tempSensor.wakeUp();
    }