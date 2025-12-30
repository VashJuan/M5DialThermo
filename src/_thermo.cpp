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
#include <esp_task_wdt.h>

#include "secrets.h"
#include "encoder.hpp"
#include "rtc.hpp"
#include "temp_sensor.hpp"
#include "stove.hpp"
#include "display.hpp"
#include "lora_transmitter.hpp"

// Forward declarations for button handlers
void handleButtonPress();
void handleButtonRelease();

// LoRa transmitter instance and configuration
LoRaTransmitter loraTransmitter;
LoRaWANConfig loraConfig;

void setupLoRaConfig() {
    loraConfig.mode = LoRaCommunicationMode::P2P;
    loraConfig.appEUI = LORAWAN_APP_EUI;
    loraConfig.appKey = LORAWAN_APP_KEY;
    loraConfig.region = LORAWAN_REGION_US915;
    loraConfig.dataRate = 3;  // LORAWAN_DR_MEDIUM equivalent
    loraConfig.adaptiveDataRate = true;
    loraConfig.transmitPower = 14;
}

// LoRa module pins (adjust as needed for your setup)
const int LORA_RX_PIN = 44;  // M5Dial → Grove-Wio-E5 TX
const int LORA_TX_PIN = 43;  // M5Dial ← Grove-Wio-E5 RX

// Activity tracking variables
static uint32_t lastActivityTime = millis();
bool recentActivity = false;
int activityTimeout = 3000; // 3 seconds

// Temperature monitoring constants
static const unsigned long TEMP_POLL_INTERVAL = 2 * 60 * 1000; // 2 minutes in milliseconds
static const unsigned long TEMP_POLL_ACTIVE_INTERVAL = 5 * 1000; // 5 seconds when active
static unsigned long lastTempPoll = 0;
static bool deepPowerSaveMode = false;

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
    String formattedTime = rtc.getFormattedDate();
    
    // Check if we have a valid time or if RTC is still initializing
    if (formattedTime.startsWith("RTC not") || formattedTime.startsWith("Time unavailable")) {
        display.showText(TIME, "Initializing clock...", COLOR_WHITE);
        
        // Show error message but don't attempt WiFi reconnection to prevent watchdog timeout
        static bool errorMessageShown = false;
        if (!errorMessageShown) {
            display.showText(STATUS_AREA, "Clock not synced - restart device", COLOR_RED);
            errorMessageShown = true;
        }
        
        return -1; // Invalid time, return error code
    }
    
    // Clear any previous error messages
    static bool errorCleared = false;
    if (!errorCleared) {
        display.showText(STATUS_AREA, "");
        errorCleared = true;
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
    display.showText(TEMP, String(temperature, 1) + " F", COLOR_WHITE);

    return temperature;
}

float getCachedTemperature()
{
    float temperature = tempSensor.getLastTemperatureF();
    
    // If no cached reading is available, take a new reading
    if (isnan(temperature)) {
        return updateTemperature();
    }
    
    // Display cached temperature
    display.showText(TEMP, String(temperature, 1) + " F (cached)", COLOR_WHITE);
    return temperature;
}

bool updateStove(float temperature, int hourOfWeek, bool manualToggleRequested = false)
{
    // Handle manual toggle request
    if (manualToggleRequested)
    {
        String statusText = stove.toggleManualOverride(temperature);

        // Give audio feedback for safety override (non-blocking)
        if (statusText == "OFF (Safety)")
        {
            M5.Speaker.tone(4000, 100);
            M5.Speaker.tone(4000, 100);
        }

        statusText = "Stove: " + statusText;
        display.showText(STOVE, statusText);
    }
    else
    {
        // Run automatic temperature control logic which returns display text
        String statusText = stove.update(temperature, hourOfWeek);
        
        // Display the status text returned by stove (includes LoRa status)
        display.showText(STOVE, "Stove: " + statusText);
    }

    return (stove.getState() == STOVE_ON);
}

void setup()
{
    Serial.begin(9600);
    
    // Configure watchdog timer for longer timeout (ESP32-S3 compatible)
    esp_task_wdt_init(30, true); // 30 second timeout, panic on timeout
    esp_task_wdt_add(NULL); // Add current task to watchdog
    
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // Ensure WiFi is initially disabled to prevent background operations
    WiFi.mode(WIFI_OFF);
    yield(); // Feed watchdog

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
        Serial.println("Failed to initialize temperature sensor!\n");
        display.showText(STATUS_AREA, "Temp Sensor Init Failed.", COLOR_RED);
        // Don't block - continue with other setup
    }
    else
    {
        Serial.printf("Temperature sensor initialized successfully at 0x%02X\n", tempSensor.getI2CAddress());
        Serial.printf("Current resolution: %s\n\n", tempSensor.getResolutionString());
    }

    // Initialize stove control (loads configuration from temps.csv)
    yield(); // Feed watchdog
    Serial.println("Setting up stove control...");
    display.showText(STATUS_AREA, "Setting up stove control...");
    delay(250);
    yield(); // Feed watchdog
    stove.setup();
    
    // Initialize LoRa transmitter (optional)
    yield(); // Feed watchdog
    Serial.println("Setting up LoRa transmitter...");
    display.showText(STATUS_AREA, "Setting up LoRa...");
    delay(250);
    yield(); // Feed watchdog
    
    // Configure LoRa settings
    setupLoRaConfig();
    
    if (loraTransmitter.setup(LORA_RX_PIN, LORA_TX_PIN, loraConfig)) {
        stove.setLoRaTransmitter(&loraTransmitter);
        stove.setLoRaControlEnabled(true);
        
        // Get current mode for display
        LoRaCommunicationMode currentMode = loraTransmitter.getCurrentMode();
        String modeStr = (currentMode == LoRaCommunicationMode::P2P) ? "P2P" : "LoRaWAN";
        
        Serial.printf("LoRa transmitter initialized successfully in %s mode\n", modeStr.c_str());
        display.showText(STATUS_AREA, "LoRa ready: " + modeStr, COLOR_GREEN);
    } else {
        Serial.println("LoRa transmitter initialization failed - continuing without LoRa");
        display.showText(STATUS_AREA, "LoRa failed - local mode only", COLOR_YELLOW);
    }
    delay(1000); // Show LoRa status for a moment

    // Clear setup message and show ready status
    display.showText(STATUS_AREA, "System Ready", COLOR_MAGENTA);
    delay(500); // Brief pause to show ready message
    
    String now = rtc.getFormattedDate();
    Serial.println("Setup done at " + now);
    Serial.println();
    display.showText(TIME, now);

    // Clear status area for normal operation
    display.showText(STATUS_AREA, "");

    // Use M5's built-in button handling instead of raw GPIO interrupts
    // M5Dial button handling is done through M5.update() and M5.BtnA
    // attachInterrupt(digitalPinToInterrupt(42), buttonPressISR, FALLING);
    // attachInterrupt(digitalPinToInterrupt(42), buttonReleaseISR, RISING);

    // Note: M5Dial encoder interrupts may be handled internally by M5.Encoder
    // Button handling will use M5.BtnA with efficient checking
    Serial.println("Using M5 built-in button handling for optimal responsiveness");
    
    // Reset activity time so power save mode doesn't trigger immediately
    lastActivityTime = millis();
    
    yield(); // Feed watchdog
}

// M5 button handler functions (more reliable than raw GPIO interrupts)
void handleButtonPress() {
    recentActivity = true;
    lastActivityTime = millis();
    
    Serial.println("Button pressed - toggling manual override");
    
    // Toggle manual override
    float curTemp = tempSensor.readTemperatureFahrenheit();
    if (tempSensor.isValidReading(curTemp)) {
        String result = stove.toggleManualOverride(curTemp);
        Serial.println("Manual toggle result: " + result);
    } else {
        Serial.println("Button press ignored - invalid temperature reading");
    }
}

void handleButtonRelease() {
    recentActivity = true;
    lastActivityTime = millis();
    Serial.println("Button released");
}

// Button interrupt handler (replaces polling)
void handleButtonInterrupts() {
    // Use M5's built-in button state detection for reliability
    if (M5.BtnA.wasPressed()) {
        handleButtonPress();
    }
    
    if (M5.BtnA.wasReleased()) {
        handleButtonRelease();
    }
}

void loop()
{
    static const int sleepShort = 1; // 1 second
    static const int sleepLong = 3;  // sleep-in if no recent activity
    static long loopCounter = 0;
    // bool touch_wakeup = true; // Whether to enable touch screen wake-up

    yield(); // Feed watchdog at start of loop
    M5.update();

    unsigned long currentTime = millis();
    bool isInactive = (currentTime - lastActivityTime > activityTimeout);
    
    // Determine temperature poll interval based on activity
    unsigned long tempPollInterval = isInactive ? TEMP_POLL_INTERVAL : TEMP_POLL_ACTIVE_INTERVAL;
    
    // Check if it's time for temperature polling
    bool timeForTempPoll = (currentTime - lastTempPoll >= tempPollInterval);
    
    // Wake up sensor before temperature reading if needed
    if (timeForTempPoll && !tempSensor.getAwakeStatus()) {
        tempSensor.wakeUp();
        delay(10); // Allow sensor to stabilize
        Serial.println("Temperature sensor woken for periodic poll");
    }

    // Read current values (time always, temperature only when needed)
    static int hourOfWeek = updateTime();
    static float curTemp = 999.0; // Initialize with invalid value
    
    if (timeForTempPoll) {
        curTemp = updateTemperature();
        lastTempPoll = currentTime;
        Serial.printf("Periodic temperature poll: %.1f°F (interval: %lus)\n", 
                     curTemp, tempPollInterval / 1000);
    }

    // Skip stove control if time is not yet available
    if (hourOfWeek < 0) {
        Serial.println("Waiting for RTC initialization...");
        delay(100); // Reduced from 1000ms
        return;
    }

    // Handle button interrupts (replaces polling)
    handleButtonInterrupts();

    // Handle encoder changes using M5 built-in handling
    // M5.update() handles encoder internally, no additional processing needed
    
    // Update stove status (handles both manual and automatic modes)
    // Only update if we have a valid temperature reading
    static bool stoveOn = false;
    if (tempSensor.isValidReading(curTemp)) {
        stoveOn = updateStove(curTemp, hourOfWeek);
    }

    // For pending states, update display more frequently to show countdown
    // Also show detailed temperature information periodically
    static unsigned long lastDisplayUpdate = 0;
    static unsigned long loopCounterForDisplay = 0;
    
    if (millis() - lastDisplayUpdate > (isInactive ? 10000 : 2000)) { // Update less frequently when inactive
        String currentState = stove.getStateString();
        if (currentState.startsWith("PENDING")) {
            display.showText(STOVE, "Stove: " + currentState);
        }
        
        // Show detailed status with cached temperature during inactive periods
        if (!(loopCounterForDisplay++ % 25)) {
            float displayTemp = isInactive ? tempSensor.getLastTemperatureF() : curTemp;
            if (!isnan(displayTemp) && tempSensor.isValidReading(displayTemp)) {
                float desiredTemp = stove.getCurrentDesiredTemperature();
                float tempDiff = desiredTemp - displayTemp;
                String statusMsg = String(desiredTemp, 1) + "F target, diff " + String(tempDiff, 1) + "F";
                if (isInactive) {
                    statusMsg += " (power save)";
                }
                display.showText(STATUS_AREA, statusMsg, isInactive ? COLOR_GRAY : COLOR_BLACK);
            }
        }
        
        // Update cached temperature display during inactive periods
        if (isInactive && !isnan(tempSensor.getLastTemperatureF())) {
            unsigned long timeSinceReading = (currentTime - tempSensor.getLastReadTime()) / 1000; // seconds
            String tempStr = String(tempSensor.getLastTemperatureF(), 1) + " F";
            if (timeSinceReading > 60) {
                tempStr += " (" + String(timeSinceReading / 60) + "m ago)";
            }
            display.showText(TEMP, tempStr, COLOR_WHITE);
        }
        
        lastDisplayUpdate = millis();
    }

    // Enhanced power saving with periodic temperature monitoring
    static bool powerSaveMode = false;
    static unsigned long powerSaveModeStartTime = 0;

    if (isInactive)
    {
        if (!powerSaveMode)
        {
            setCpuFrequencyMhz(40); // Further reduce CPU frequency when idle
            powerSaveMode = true;
            powerSaveModeStartTime = millis();
            Serial.println(String(loopCounter) + ") Entering power save mode (CPU 40MHz, periodic temp polling)\n");
        }
        
        // Put sensor to sleep after temperature reading (if we just polled)
        if (timeForTempPoll && tempSensor.getAwakeStatus()) {
            tempSensor.getLastTemperatureF(); // Cache the last temperature reading
            delay(100); // Allow any pending operations to complete
            tempSensor.shutdown();
            Serial.printf("Temperature sensor shutdown after poll at %s. Sleeping for 2 minutes...\n", rtc.getFormattedTime().c_str());
        }
        
        // Enter deep power save mode after being in power save mode for at least 30 seconds
        if (!deepPowerSaveMode && (millis() - powerSaveModeStartTime > 30000)) {
            deepPowerSaveMode = true;
            Serial.println("Entering deep power save mode - temperature polling every 2 minutes");
        }
        
        delay(1000); // Sleep longer between loops when inactive
    }
    else
    {
        if (powerSaveMode)
        {
            setCpuFrequencyMhz(80); // Return to normal power saving frequency
            powerSaveMode = false;
            deepPowerSaveMode = false;
            powerSaveModeStartTime = 0;
            Serial.println("Exiting power save mode (CPU 80MHz, active temp monitoring)");
        }
        
        // Ensure sensor is awake during active periods
        if (!tempSensor.getAwakeStatus()) {
            tempSensor.wakeUp();
            Serial.println("Temperature sensor woken for active period");
        }
    }

    // Note: stove status display is handled inside updateStove()
    // Note: M5Dial doesn't have PortB, use direct GPIO control\n
    // For now, just print stove status - actual GPIO control would need specific pin setup

    // Adaptive delay based on power mode
    if (deepPowerSaveMode) {
        delay(500); // Longer sleep in deep power save mode
    } else if (powerSaveMode) {
        delay(100); // Medium sleep in power save mode
    } else {
        delay(50);  // Short sleep during active periods
    }
    
    // Periodic watchdog feeding at loop end
    esp_task_wdt_reset();
    
    loopCounter++;
}
