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

// #include <Arduino.h>
#include <M5Dial.h>
#include "encoder.hpp"
#include "rtc.hpp"
#include "temp_sensor.hpp"
#include "stove.hpp"

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
// Default I2C address is 0x18, but can be configured to 0x18-0x1F
// Connect to Port A (G15=SCL, G13=SDA) on M5Dial

/** per https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html,
 * https://files.seeedstudio.com/products/317990687/res/LoRa-E5%20module%20datasheet_V1.1.pdf, pg 6
 *  PB15 I/O SCL of I2C2 from MCU
 * PA15 I/O SDA of I2C2 from MCU
 */
const int grove_wio_e5_sensor_yellow = portB[0]; // TX
const int grove_wio_e5_sensor_white = portB[1];  // RX
// These are returning 0 for me!
const int display_center_x = (M5Dial.Display.width() == 0 ? 240 : M5Dial.Display.width()) / 2;
const int display_center_y = (M5Dial.Display.height() == 0 ? 240 : M5Dial.Display.height()) / 2;
int display_offset_y = 40; // start at top usable line on round display
const uint32_t display_background_color = 0xFFB040;
void splashScreen()
{
    Serial.println("\n\n------------------------------");
    // Serial.printf("display_center_x = %i; display_center_y = %i; display_offset_y = %i\n", display_center_x, display_center_y, display_offset_y);
    M5Dial.Display.clear();
    M5Dial.Display.fillScreen(display_background_color);
    // M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextColor(TFT_BLACK);
    M5Dial.Display.setFont(&fonts::Font2); // https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx/v1/lgfx_fonts.hpp
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_center); // https://doc-tft-espi.readthedocs.io/tft_espi/datums/
    M5Dial.Display.drawCenterString("M5Dial Thermostat v 2.0.0", display_center_x, display_offset_y);
    M5Dial.Display.drawLine(20, display_offset_y + 15, 220, display_offset_y += 15, TFT_WHITE);

    // M5Dial.Display.setCursor(20, 40);
    delay(100);
}

void setup()
{
    Serial.begin(9600);
    auto cfg = m5Dial.config();
    m5Dial.begin(cfg, true, false);

    splashScreen();

    Serial.println("Setting up encoder and RTC...");
    encoder.setup();
    rtc.setup();

    // Initialize temperature sensor
    if (!tempSensor.setup())
    {
        Serial.println("Failed to initialize temperature sensor!");
        m5Dial.Display.clear();
        m5Dial.Display.setTextColor(RED);
        m5Dial.Display.drawCenterString("Temp Sensor Init Failed.", display_center_x, display_offset_y += 15);
        // Don't block - continue with other setup
    }
    else
    {
        Serial.printf("Temperature sensor initialized successfully at 0x%02X\n", tempSensor.getI2CAddress());
        Serial.printf("Current resolution: %s\n", tempSensor.getResolutionString());
    }

    Serial.println("Setup done.");
}

static uint32_t lastActivityTime = millis();
bool recentActivity = false;
int activityTimeout = 3000; // 5 seconds

void noteActivity()
{
    recentActivity = true;
    lastActivityTime = millis();
}

int sleepShort = 1; // 1 second
int sleepLong = 4;  // sleep-in if no recent activity
int loopCounter = 0;

void loop()
{
    m5Dial.update();
    // encoder.update();
    rtc.update();

    if (encoder.hasPositionChanged())
    {
        long position = encoder.getPosition();
        Serial.printf("Encoder position: %ld\n", position);
        noteActivity();
    }

    if (m5Dial.BtnA.wasPressed())
    {
        m5Dial.Speaker.tone(8000, 20);
        noteActivity();
    }

    if (m5Dial.BtnA.wasReleased())
    {
        m5Dial.Speaker.tone(12000, 20);
        delay(500);
        // m5Dial.Speaker.mute();
        //  m5Dial.Display.clear();
        //  m5Dial.Display.drawCenterString("Released", display_center_x, display_offset_y + 20);
        noteActivity();
    }

    // Read temperature sensor
    float temperature = tempSensor.readTemperatureFahrenheit();

    if (!tempSensor.isValidReading(temperature))
    {
        Serial.println("Invalid temperature reading");
        m5Dial.Display.setTextColor(RED);
        m5Dial.Display.drawCenterString("Temperature Sensor Error", display_center_x, display_offset_y += 15);
        m5Dial.Display.setTextColor(TFT_BLACK);
    }
    else
    {
        // Serial.printf("Temperature: %.2f°F\n", temperature);
        m5Dial.Display.setTextColor(TFT_BLACK);
        // clear previous temperature display
        m5Dial.Display.fillRect(0, display_offset_y + 5, m5Dial.Display.width(), display_offset_y + 10, display_background_color);
        m5Dial.Display.drawCenterString(String(temperature, 1) + " F", display_center_x, display_offset_y); // '°' isn't available; overwrite previous values
    }

    // Signal stove to turn on/off based on the current temperature
    String stoveStatus = stove.update(tempSensor, rtc);
    if (stoveStatus.length() > 0)
    {
        m5Dial.Display.drawCenterString(stoveStatus, display_center_x, display_offset_y += 15);
    }

    // Save battery power: sleep awhile, or until activity noted
    // https://deepwiki.com/m5stack/M5Unified/4.3-sleep-modes-and-power-off#wakeup-pin-configuration
    if (loopCounter++ % 100 == 0)
    {
        Serial.println(loopCounter + ") Sleeping for " + String((millis() - lastActivityTime > activityTimeout) ? sleepLong : sleepShort) + " seconds...");
    }
    // tempSensor.shutdown();

    // M5.Power.lightSleep(((millis() - lastActivityTime > activityTimeout) ? sleepLong : sleepShort) * 1000000ULL, true);
    // tempSensor.wakeUp();

    // m5Dial.wakeUp();
}

// Declare the external M5Dial object
extern M5Dial m5Dial;