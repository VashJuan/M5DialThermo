/**
 * @file stove.cpp
 * @brief Stove Control Class Implementation
 * @version 1.0
 * @date 2025-12-17
 *
 * @Hardware: M5Dial with relay control for stove/heater
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */


// https://docs.m5stack.com/en/arduino/m5unified/imu_class

#include <M5Unified.h>
#include <FS.h>
#include <SPIFFS.h>
#include "stove.hpp"

// Global instance for easy access
Stove stove;

// Safety maximum temperature
const float Stove::SAFETY_MAX_TEMP = 82.0;

Stove::Stove(int pin, float baseTemp) : 
    relayPin(pin), 
    currentState(STOVE_OFF), 
    lastStateChange(0),
    minChangeInterval(30000), // 30 seconds in milliseconds
    enabled(true),
    manualOverride(false)
{
    // Initialize timeOffset array with default values as fallback
    timeOffset[0] = 0.0;   // Index 0 - unused
    for (int i = 1; i <= 24; i++) {
        timeOffset[i] = -5.0;  // Default fallback offset
    }
    
    // Try to load configuration from CSV file
    bool csvLoaded = loadConfigFromCSV();
    
    // Set base temperature
    if (baseTemp >= 0) {
        // Use provided temperature
        baseTemperature = baseTemp;
    } else if (csvLoaded) {
        // baseTemperature was set by loadConfigFromCSV
        // No action needed
    } else {
        // Fallback default
        baseTemperature = 68.0;
    }
}

Stove::~Stove()
{
    // Turn off stove when object is destroyed
    setRelayState(false);
}

bool Stove::loadConfigFromCSV()
{
    // Try to open the temps.csv file from SPIFFS (internal flash)
    File file;
    
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("Warning: Failed to mount SPIFFS filesystem");
        return false;
    }
    
    // Try to open from SPIFFS
    file = SPIFFS.open("/temps.csv", "r");
    if (!file) {
        // Try without leading slash
        file = SPIFFS.open("temps.csv", "r");
    }
    
    if (!file) {
        Serial.println("Warning: Could not open temps.csv from SPIFFS, using default values");
        return false;
    }

    Serial.println("Loading configuration from temps.csv");
    
    String line;
    bool baseTemperatureSet = false;
    
    // Initialize timeOffset array with defaults
    timeOffset[0] = 0.0;  // Index 0 - unused
    for (int i = 1; i <= 24; i++) {
        timeOffset[i] = -5.0;  // Default fallback
    }
    
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        
        // Skip comments and empty lines
        if (line.length() == 0 || line.startsWith("#") || line.startsWith("Hour,")) {
            continue;
        }
        
        // Parse base temperature
        if (line.startsWith("BaseTemperature,")) {
            int commaIndex = line.indexOf(',');
            if (commaIndex != -1) {
                String tempStr = line.substring(commaIndex + 1);
                baseTemperature = tempStr.toFloat();
                baseTemperatureSet = true;
                Serial.printf("Loaded base temperature: %.1f°F\n", baseTemperature);
                continue;
            }
        }
        
        // Parse hourly offsets (format: Hour,Offset,Description)
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        
        if (firstComma != -1 && secondComma != -1) {
            String hourStr = line.substring(0, firstComma);
            String offsetStr = line.substring(firstComma + 1, secondComma);
            
            int hour = hourStr.toInt();
            float offset = offsetStr.toFloat();
            
            if (hour >= 1 && hour <= 24) {
                timeOffset[hour] = offset;
                Serial.printf("Hour %d: %.1f°F offset\n", hour, offset);
            }
        }
    }
    
    file.close();
    
    if (!baseTemperatureSet) {
        Serial.println("Warning: Base temperature not found in CSV, using default 68.0°F");
        baseTemperature = 68.0;
        return false;
    }
    
    Serial.println("Successfully loaded temperature configuration from temps.csv");
    return true;
}

void Stove::setup()
{
    pinMode(relayPin, OUTPUT);
    setRelayState(false); // Start with stove off
    lastStateChange = millis();
    
    Serial.printf("Stove control initialized on pin %d, base temperature: %.1f°F\n", 
                  relayPin, baseTemperature);
    Serial.println("Temperature schedule loaded from temps.csv (or defaults if file not found)");
}

float Stove::getTemperatureAdjustment(int hour)
{
    if (hour < 1 || hour > 24) {
        return 0.0; // No adjustment for invalid hours
    }
    return timeOffset[hour];
}

bool Stove::canChangeState()
{
    return (millis() - lastStateChange) >= minChangeInterval;
}

void Stove::setRelayState(bool on)
{
    digitalWrite(relayPin, on ? HIGH : LOW);
    Serial.printf("Relay set to: %s\n", on ? "ON" : "OFF");
}

String Stove::update(float currentTemp, int hourOfWeek)
{
    String status = "";
    static unsigned long loopCounter = 0;

    // Debug: Show that update method is being called
    Serial.printf("DEBUG: Stove::update called - enabled=%s, manualOverride=%s\n", 
                  enabled ? "true" : "false", manualOverride ? "true" : "false");

    // If manual override is active, don't run automatic control
    if (manualOverride) {
        Serial.println("DEBUG: Manual override active, skipping automatic control");
        return (currentState == STOVE_ON) ? "ON" : "OFF";
    }

    if (!enabled) {
        Serial.println("Stove: not enabled");
        return "Stove: not enabled";
    }
        
    // Get desired temperature for current time
    float desiredTemp = getDesiredTemperature(rtc);
    
    // Calculate temperature difference
    float tempDiff = desiredTemp - currentTemp;
    
    // Show temperature info every 10 loop iterations (more frequent)
    //if (!(loopCounter % 10))
     {
        Serial.printf("%lu) Temp: Current=%.1f°F, Target=%.1f°F, Diff=%.1f°F, State=%s\n", 
                      loopCounter++, currentTemp, desiredTemp, tempDiff, getStateString().c_str());
    }
    
    // Determine if state change is needed
    bool shouldBeOn = false;
    
    // Hysteresis: different thresholds for turning on vs off to prevent oscillation
    if (currentState == STOVE_OFF || currentState == STOVE_PENDING_OFF) {
        // Turn on if temperature is below desired
        shouldBeOn = (tempDiff >= STOVE_HYSTERESIS_LOW);
    } else {
        // Turn off if temperature is at or above desired
        shouldBeOn = (tempDiff > STOVE_HYSTERESIS_HIGH);
    }
    
    // Apply state change if needed and allowed
    if (shouldBeOn && (currentState == STOVE_OFF || currentState == STOVE_PENDING_OFF)) {
        if (canChangeState()) {
            status = turnOn();
        } else {
            currentState = STOVE_PENDING_ON;
            Serial.println("Stove: Pending turn ON (waiting for minimum interval)");
        }
    } else if (!shouldBeOn && (currentState == STOVE_ON || currentState == STOVE_PENDING_ON)) {
        if (canChangeState()) {
            status = turnOff();
        } else {
            currentState = STOVE_PENDING_OFF;
            Serial.println("Stove: Pending turn OFF (waiting for minimum interval)");
        }
    }
    
    // Handle pending states
    if (currentState == STOVE_PENDING_ON && canChangeState()) {
        status = turnOn();
    } else if (currentState == STOVE_PENDING_ON && !canChangeState()) {
        // Show remaining time for pending state
        unsigned long remainingSeconds = getTimeUntilNextChange();
        Serial.printf("Stove: PENDING_ON, %lu seconds until ON\n", remainingSeconds);
    } else if (currentState == STOVE_PENDING_OFF && canChangeState()) {
        status = turnOff();
    }
    
    return status;
}

String Stove::turnOn()
{
    if (!canChangeState() && currentState != STOVE_PENDING_ON) {
        Serial.printf("Stove: Cannot turn on, %lu seconds remaining\n", 
                      getTimeUntilNextChange());
        return "";
    }
    
    currentState = STOVE_ON;
    lastStateChange = millis();
    setRelayState(true);
    Serial.println("Stove: Turned ON");
    return "Stove: Turned ON";
}

String Stove::turnOff()
{
    if (!canChangeState() && currentState != STOVE_PENDING_OFF) {
        Serial.printf("Stove: Cannot turn off, %lu seconds remaining\n", 
                      getTimeUntilNextChange());
        return "";
    }
    
    currentState = STOVE_OFF;
    lastStateChange = millis();
    setRelayState(false);
    Serial.println("Stove: Turned OFF");
    return "Stove: Turned OFF";
}

StoveState Stove::getState() const
{
    return currentState;
}

float Stove::getDesiredTemperature(RTC &rtc)
{
    int currentHour = rtc.getHour();
    float adjustment = getTemperatureAdjustment(currentHour);
    return baseTemperature + adjustment;
}

void Stove::setBaseTemperature(float temp)
{
    baseTemperature = temp;
    Serial.printf("Stove: Base temperature set to %.1f°F\n", baseTemperature);
}

float Stove::getBaseTemperature() const
{
    return baseTemperature;
}

void Stove::setEnabled(bool enable)
{
    enabled = enable;
    Serial.printf("Stove: Automatic control %s\n", enable ? "ENABLED" : "DISABLED");
    
    if (!enable && (currentState == STOVE_ON)) {
        // Turn off stove when disabling automatic control
        forceState(false);
    }
}

bool Stove::isEnabled() const
{
    return enabled;
}

unsigned long Stove::getTimeUntilNextChange() const
{
    unsigned long elapsed = millis() - lastStateChange;
    if (elapsed >= minChangeInterval) {
        return 0;
    }
    return (minChangeInterval - elapsed) / 1000; // Return in seconds
}

String Stove::getStateString() const
{
    switch (currentState) {
        case STOVE_OFF:         return "OFF";
        case STOVE_ON:          return "ON";
        case STOVE_PENDING_ON:  {
            unsigned long remainingSeconds = getTimeUntilNextChange();
            return "ON in " + String(remainingSeconds) + "s";
        }
        case STOVE_PENDING_OFF: {
            unsigned long remainingSeconds = getTimeUntilNextChange();
            return "OFF in " + String(remainingSeconds) + "s";
        }
        default:                return "UNKNOWN";
    }
}

void Stove::forceState(bool on)
{
    Serial.printf("Stove: FORCE state to %s\n", on ? "ON" : "OFF");
    
    currentState = on ? STOVE_ON : STOVE_OFF;
    lastStateChange = millis();
    setRelayState(on);
}

String Stove::toggleManualOverride(float currentTemp)
{
    if (!manualOverride) {
        // Turning ON: check safety temperature limit
        if (currentTemp <= SAFETY_MAX_TEMP) {
            manualOverride = true;
            forceState(true);
            Serial.println("Manual stove override ON");
            return "MANUAL ON";
        } else {
            Serial.printf("Safety: Cannot turn on stove - temperature %.1f°F exceeds safety limit of %.1f°F\n", 
                         currentTemp, SAFETY_MAX_TEMP);
            return "OFF (Safety)";
        }
    } else {
        // Turning OFF
        manualOverride = false;
        forceState(false);
        Serial.println("Manual stove override OFF");
        return "OFF";
    }
}

bool Stove::isManualOverride() const
{
    return manualOverride;
}

String Stove::getStatus(float currentTemp, int hourOfWeek)
{
    if (manualOverride) {
        return "MANUAL ON";
    } else {
        // Use existing update logic but just return status
        String autoStatus = update(currentTemp, hourOfWeek);
        if (autoStatus == "ON") {
            return "AUTO ON";
        } else {
            return "OFF";
        }
    }
}

void Stove::clearManualOverride()
{
    if (manualOverride) {
        manualOverride = false;
        Serial.println("Manual override cleared - returning to automatic mode");
        // Don't immediately change state, let automatic control take over on next update
    }
}

