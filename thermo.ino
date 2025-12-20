/**
 * @file thermo.ino
 * @url https://github.com/vashjuan/M5Dial_thermo
 * @author John Cornelison (john@vashonSoftware.com)
 * @brief M5Dial thermo
 * @version 2.0.0
 * @date 2025-12-18
 *
 *
 * @Hardware:  M5Dial
 *               (https://m5stack.com/products/m5dial)
 *               which uses the M5StampS3 as the controller
 *               (https://shop.m5stack.com/products/m5stamp-esp32s3-module)
 *               https://docs.m5stack.com/en/arduino/m5stamps3/program
 *             + Grove - Temperature Sensor V1.2
 *               (https://www.seeedstudio.com/Grove-Temperature-Sensor.html) &
 *               (https://wiki.seeedstudio.com/Grove-Temperature_Sensor_V1.2/)
 *             + Grove-Wio-E5 Wireless Module - STM32WLE5JC, ARM Cortex-M4 and SX126x; EU868 & US915LoRaWAN
 *               (https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html)
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 *
 * M5StampS3A, is an update to the M5Stamp S3 ESP32-S3 module introduced in 2023 with
 * an optimized antenna design, lower power consumption, a larger user button, and
 * a different logic for the RGB LED control.
 * - https://www.cnx-software.com/2025/07/11/m5stack-stamp-s3a-wifi-and-ble-iot-module-benefits-from-optimized-antenna-design-lower-power-consumption/
 **/

#include "M5Dial.h"
#include "encoder.hpp"
#include "rtc.hpp"
#include "temp_sensor.hpp"
// #include "stove.hpp"

/**
 * per https://docs.m5stack.com/en/core/M5Dial#pinmap:
 * https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/684/S007_PinMap_01.jpg
 *  White: M5Dial pins G15 SCL (serial clock) (for Port A) & G1 (for Port B)
 *  Yellow: M5Dial pins G13 SDA (serial data) (for Port A) & G2 (for Port B)
 *  Red: 5 volts
 *  Black: ground
 */
const String portA[] = {"15", "13"}; // G15 = SCL, G13 = SDA
const String portB[] = {"1", "2"};   // G1 = SCL, G2 = SDA

// per https://wiki.seeedstudio.com/Grove-Temperature_Sensor_V1.2/#hardware
// & https://www.seeedstudio.com/Grove-Temperature-Sensor.html
const String grove_temp_sensor_yellow = portA[1];
const String grove_temp_sensor_white = "unconnected";

/** per https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html,
 * https://files.seeedstudio.com/products/317990687/res/LoRa-E5%20module%20datasheet_V1.1.pdf, pg 6
 *  PB15 I/O SCL of I2C2 from MCU
 * PA15 I/O SDA of I2C2 from MCU
 */
const String grove_wio_e5_sensor_yellow = portB[0]; // TX
const String grove_wio_e5_sensor_white = portB[1];  // RX

void splashScreen()
{
    M5Dial.Display.clear();
    M5Dial.Display.fillScreen(0xFFB040); // Amber color
    M5Dial.Display.setTextColor(BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("M5Dial Fireside Thermostat v 1.0.0", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);

    M5Dial.Display.drawString(".(0,50)", 0, 50);
    M5Dial.Display.drawString(".(50,50)", 50, 50);
    M5Dial.Display.drawString(".(100,100)", 100, 100);
    M5Dial.Display.drawString(".(100,200)", 100, 200);
    M5Dial.Display.drawString(".(200,100)", 200, 100);
    M5Dial.Display.drawString(".(200,200)", 200, 200);

    delay(1000);
}

void setup()
{
    Serial.begin(9600);
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setFont(&fonts::Font4); // https://github.com/lovyan03/LovyanGFX/blob/master/src/lgfx/v1/lgfx_fonts.hpp
    // M5Dial.Display.setTextSize(1);

    splashScreen();

    M5Dial.Display.setTextSize(1);
    splashScreen();

    encoder.setup();
    rtc.setup();
    tempSensor.setup();
    tempSensor.setSensorPin(grove_temp_sensor_yellow); // ADC1_0 (GPIO36)

    Serial.println("Setup done.");
}

static uint32_t lastActivityTime = millis();
bool recentActivity = false;
int activityTimeout = 5000; // 5 seconds

void noteActivity()
{
    recentActivity = true;
    lastActivityTime = millis();
}

int sleepShort = 1; // 1 second
int sleepLong = 10; // sleep-in if no recent activity

void loop()
{
    M5Dial.update();
    // encoder.update();
    rtc.update();

    if (encoder.hasPositionChanged())
    {
        long position = encoder.getPosition();
        Serial.printf("Encoder position: %ld\n", position);
        noteActivity();
    }

    if (M5Dial.BtnA.wasPressed())
    {
        M5Dial.Speaker.tone(8000, 20);
        noteActivity();
    }

    if (M5Dial.BtnA.wasReleased())
    {
        M5Dial.Speaker.tone(12000, 20);
        delay(500);
        // M5Dial.Speaker.mute();
        //  M5Dial.Display.clear();
        //  M5Dial.Display.drawString("Released", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        noteActivity();
    }

    // Read temperature sensor
    float temperature = tempSensor.readTemperatureFahrenheit();

    if (!tempSensor.isValidReading(temperature))
    {
        Serial.println("Invalid temperature reading");
        M5Dial.Display.setTextColor(RED);
        M5Dial.Display.drawString("Temperature Sensor Error", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 40);
    }
    else
    {
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.drawString(String(temperature) + " °F", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 40);
    }

    //

    // To save battery power, sleep some seconds (or until buttons cause wakeup)
    // https://deepwiki.com/m5stack/M5Unified/4.3-sleep-modes-and-power-off#wakeup-pin-configuration
    M5.Power.lightSleep(((millis() - lastActivityTime > activityTimeout) ? sleepLong : sleepShort) * 1000000ULL, true);

    // M5Dial.wakeUp();
}