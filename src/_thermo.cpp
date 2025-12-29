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
#include <Arduino.h>
// System includes must be first
#include <M5Unified.h>

#include "encoder.hpp"
#include "rtc.hpp"
#include "temp_sensor.hpp"
#include "stove.hpp"
#include "display.hpp"

// Forward declarations for ISR functions
void IRAM_ATTR buttonPressISR();
void IRAM_ATTR buttonReleaseISR();

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
    String formattedTime = rtc.getFormattedTime();
    
    // Check if we have a valid time or if RTC is still initializing
    if (formattedTime.startsWith("RTC not") || formattedTime.startsWith("Time unavailable")) {
        display.showText(TIME, "Initializing clock...", COLOR_WHITE);
        return -1; // Invalid time, return error code
    }
    
    display.showText(TIME, formattedTime);
    return rtc.getDayOfWeek() * 24 + rtc.getHour(); // hour of the week
}

float updateTemperature()
{
    float temperature = tempSensor.readTemperatureFahrenheit();

    if (!tempSensor.isValidReading(temperature))
    {
        Serial.println("Invalid temperature reading");
        display.showText(STATUS_AREA, "Temperature Sensor Error", COLOR_RED);
        return 999.0;
    }

    // Serial.printf("Temperature: %.2f°F\n", temperature);
    display.showText(TEMP, String(temperature, 1) + " F");

    return temperature;
}

bool updateStove(float temperature, int hourOfWeek, bool manualToggleRequested = false)
{
    String statusText = "";

    // Handle manual toggle request
    if (manualToggleRequested)
    {
        statusText = stove.toggleManualOverride(temperature);

        // Give audio feedback for safety override
        if (statusText == "OFF (Safety)")
        {
            M5.Speaker.tone(4000, 100);
            delay(100);
            M5.Speaker.tone(4000, 100);
        }

        statusText = "Stove: " + statusText;
    }
    else
    {
        // Get current status
        statusText = "Stove: " + stove.getStatus(temperature, hourOfWeek);
    }

    // Display status
    display.showText(STOVE, statusText);

    return (stove.getState() == STOVE_ON);
}

void setup()
{
    Serial.begin(9600);
    auto cfg = M5.config();
    M5.begin(cfg);

    display.setup();
    display.showSplashScreen();
    yield(); // Feed watchdog

    Serial.print("Setting up encoder...");
    display.showText(TIME, "Setting up (encoder) dial...");
    delay(250);
    yield(); // Feed watchdog
    encoder.setup();

    Serial.println(" and RTC...");
    display.showText(TIME, "Setting up real time clock...");
    delay(250);
    yield(); // Feed watchdog
    rtc.setup();

    // Initialize temperature sensor
    yield(); // Feed watchdog
    if (!tempSensor.setup())
    {
        Serial.println("Failed to initialize temperature sensor!");
        display.showText(STATUS_AREA, "Temp Sensor Init Failed.", COLOR_RED);
        // Don't block - continue with other setup
    }
    else
    {
        Serial.printf("Temperature sensor initialized successfully at 0x%02X\n", tempSensor.getI2CAddress());
        Serial.printf("Current resolution: %s\n", tempSensor.getResolutionString());
    }

    // Initialize stove control (loads configuration from temps.csv)
    yield(); // Feed watchdog
    Serial.println("Setting up stove control...");
    display.showText(STATUS_AREA, "Setting up stove control...");
    delay(250);
    yield(); // Feed watchdog
    stove.setup();

    String now = rtc.getFormattedTime();
    Serial.println("Setup done at " + now);
    display.showText(TIME, now);

    // Setup interrupts for responsive user input
    // M5Dial button is typically on GPIO42, encoder on GPIO40/41
    attachInterrupt(digitalPinToInterrupt(42), buttonPressISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(42), buttonReleaseISR, RISING);

    // Note: M5Dial encoder interrupts may be handled internally by M5.Encoder
    // If not, you would attach to GPIO40 and GPIO41
    Serial.println("Interrupts configured for responsive input");
    yield(); // Feed watchdog
}

static uint32_t lastActivityTime = millis();
bool recentActivity = false;
int activityTimeout = 3000; // 3 seconds

// Interrupt flags (volatile for ISR communication)
volatile bool encoderChanged = false;
volatile bool buttonPressed = false;
volatile bool buttonReleased = false;
volatile unsigned long lastButtonTime = 0;

// Interrupt Service Routines
void IRAM_ATTR encoderISR()
{
    encoderChanged = true;
    recentActivity = true;
    lastActivityTime = millis();
}

void IRAM_ATTR buttonPressISR()
{
    unsigned long currentTime = millis();
    if (currentTime - lastButtonTime > 50)
    { // Debounce 50ms
        buttonPressed = true;
        recentActivity = true;
        lastActivityTime = currentTime;
        lastButtonTime = currentTime;
    }
}

void IRAM_ATTR buttonReleaseISR()
{
    unsigned long currentTime = millis();
    if (currentTime - lastButtonTime > 50)
    { // Debounce 50ms
        buttonReleased = true;
        recentActivity = true;
        lastActivityTime = currentTime;
        lastButtonTime = currentTime;
    }
}

void noteActivity()
{
    recentActivity = true;
    lastActivityTime = millis();
}

void loop()
{
    static const int sleepShort = 1; // 1 second
    static const int sleepLong = 3;  // sleep-in if no recent activity
    static long loopCounter = 0;
    // bool touch_wakeup = true; // Whether to enable touch screen wake-up

    yield(); // Feed watchdog at start of loop
    M5.update();
    // encoder.update();

    // Read current values first so they're available for all handlers
    static int hourOfWeek = updateTime();
    static float curTemp = updateTemperature();

    // Skip stove control if time is not yet available
    if (hourOfWeek < 0) {
        Serial.println("Waiting for RTC initialization...");
        delay(100); // Reduced from 1000ms
        yield(); // Feed watchdog
        return;
    }

    yield(); // Feed watchdog before handlers

    // Handle encoder changes (interrupt-driven)
    if (encoderChanged)
    {
        encoderChanged = false; // Clear flag
        long position = encoder.getPosition();
        Serial.printf("Encoder position: %ld\n", position);
        // Note: noteActivity() already called in ISR
    }

    // Handle button press (interrupt-driven)
    if (buttonPressed)
    {
        buttonPressed = false; // Clear flag
        M5.Speaker.tone(8000, 20);
        // Removed blocking delay

        // Request manual toggle through updateStove function
        updateStove(curTemp, hourOfWeek, true);
        // Note: noteActivity() already called in ISR
    }

    // Handle button release (interrupt-driven)
    if (buttonReleased)
    {
        buttonReleased = false; // Clear flag
        M5.Speaker.tone(12000, 20);
        // Removed blocking delay
        // Note: noteActivity() already called in ISR
    }

    // Update stove status (handles both manual and automatic modes)
    bool stoveOn = updateStove(curTemp, hourOfWeek);

    // Save battery power with display-friendly approach
    // Use CPU frequency scaling and WiFi/Temp Sensor sleep instead of deep sleep
    static bool powerSaveMode = false;

    if (millis() - lastActivityTime > activityTimeout)
    {
        if (!powerSaveMode)
        {
            setCpuFrequencyMhz(40); // Further reduce CPU frequency when idle
            tempSensor.shutdown();  // Shutdown sensor during idle periods
            powerSaveMode = true;
            Serial.println(String(loopCounter) + ") Entering power save mode (CPU 40MHz, sensor shutdown)");
        }
    }
    else
    {
        if (powerSaveMode)
        {
            setCpuFrequencyMhz(80); // Return to normal power saving frequency
            tempSensor.wakeUp();    // Wake up sensor when activity resumes
            powerSaveMode = false;
            Serial.println("Exiting power save mode (CPU 80MHz, sensor active)");
        }
    }

    // Note: stove status display is handled inside updateStove()
    // Note: M5Dial doesn't have PortB, use direct GPIO control\n
    // For now, just print stove status - actual GPIO control would need specific pin setup
    if (!(loopCounter++ % 100)) {
        Serial.println(String(loopCounter) + ") Stove control: " + (stoveOn ? "ON" : "OFF"));
    }

    yield(); // Feed watchdog before delay
    delay(100); // Short delay to prevent excessive looping, interrupts still responsive
}
