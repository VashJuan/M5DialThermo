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
#include "../shared/protocol_common.hpp"

// Global instance for easy access
Stove stove;

// External reference to global RTC instance from main
extern RTC rtc;

// Safety maximum temperature
const float Stove::SAFETY_MAX_TEMP = 82.0;

Stove::Stove(int pin, float baseTemp) : 
    relayControl(pin, 180000, "Stove"),  // Initialize relay control with 3-minute interval
    currentState(STOVE_OFF), 
    lastStateChange(0),
    minChangeInterval(180000), // 3 minutes delay between state changes
    enabled(true),
    manualOverride(false),
    loraControlEnabled(false)
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
    // RelayControl destructor will handle relay cleanup
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
    relayControl.setup(); // Initialize relay control
    currentState = STOVE_OFF;
    lastStateChange = millis();
    
    Serial.printf("Stove control initialized with relay on pin %d, base temperature: %.1f°F\n", 
                  2, baseTemperature); // TODO: Get actual pin from relayControl if needed
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
    return relayControl.canChangeState();
}

void Stove::setRelayState(bool on)
{
    // Use relay control for state management
    if (on) {
        relayControl.forceState(true);
    } else {
        relayControl.forceState(false);
    }
}

String Stove::update(float currentTemp, int hourOfWeek)
{
    String status = "";
    static unsigned long loopCounter = 0;

    // Debug: Show that update method is being called
    // Serial.printf("Debug: Stove::update called - enabled=%s, manualOverride=%s\n", enabled ? "true" : "false", manualOverride ? "true" : "false");

    // If manual override is active, don't run automatic control
    if (manualOverride) {
        if (!(loopCounter % 500)) {
            Serial.println("DEBUG: Manual override active, skipping automatic control");
        }
        return (currentState == STOVE_ON) ? "ON" : "OFF";
    }

    if (!enabled) {
        Serial.println("Stove: not enabled");
        return "Stove: not enabled";
    }
        
    float desiredTemp = getDesiredTemperature(rtc);   
    float tempDiff = desiredTemp - currentTemp;
    
    if (!(loopCounter % 100))
     {
        Serial.printf("%lu) Temp: Current=%.1f°F, Target=%.1f°F, Diff=%.1f°F, State=%s\n", 
                      loopCounter, currentTemp, desiredTemp, tempDiff, getStateString().c_str());
    }
    
    // Feed watchdog during intensive stove operations
    yield();
    
    bool shouldBeOn = false;
    
    // Hysteresis: different thresholds for turning on vs off to prevent oscillation
    if (currentState == STOVE_OFF || currentState == STOVE_PENDING_OFF) {
        // Turn on if temperature is below desired
        shouldBeOn = (tempDiff >= STOVE_HYSTERESIS_LOW);
    } else {
        // Turn off if temperature is at or above desired
        shouldBeOn = (tempDiff > STOVE_HYSTERESIS_HIGH);
    }
    
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
        //if (!(loopCounter % 100)) {
        //    Serial.printf("Stove: PENDING_ON, %lu seconds until ON\n", remainingSeconds);
        //}
    } else if (currentState == STOVE_PENDING_OFF && canChangeState()) {
        status = turnOff();
    }
    
    // Increment loop counter (will naturally overflow and wrap to 0)
    loopCounter++;
    
    return status;
}

String Stove::turnOn()
{
    if (!canChangeState() && currentState != STOVE_PENDING_ON) {
        Serial.printf("Stove: Cannot turn on, %lu seconds remaining\n", 
                      getTimeUntilNextChange());
        return "";
    }
    
    String result = relayControl.turnOn();
    if (result.indexOf("ON") >= 0) {
        currentState = STOVE_ON;
        lastStateChange = millis();
        Serial.println("Stove: Turned ON");
        return "Stove: Turned ON";
    }
    
    return result;
}

String Stove::turnOff()
{
    if (!canChangeState() && currentState != STOVE_PENDING_OFF) {
        Serial.printf("Stove: Cannot turn off, %lu seconds remaining\n", 
                      getTimeUntilNextChange());
        return "";
    }
    
    String result = relayControl.turnOff();
    if (result.indexOf("OFF") >= 0) {
        currentState = STOVE_OFF;
        lastStateChange = millis();
        Serial.println("Stove: Turned OFF");
        return "Stove: Turned OFF";
    }
    
    return result;
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

float Stove::getCurrentDesiredTemperature()
{
    return getDesiredTemperature(rtc);
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
    return relayControl.getTimeUntilNextChange();
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
    
    relayControl.forceState(on);
    currentState = on ? STOVE_ON : STOVE_OFF;
    lastStateChange = millis();
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

void Stove::setLoRaControlEnabled(bool enable)
{
    loraControlEnabled = enable;
    relayControl.setRemoteControlEnabled(enable);
    Serial.printf("Stove: LoRa remote control %s\n", enable ? "ENABLED" : "DISABLED");
}

bool Stove::isLoRaControlEnabled() const
{
    return loraControlEnabled;
}

String Stove::processLoRaCommand(const String& command, float currentTemp)
{
    if (!loraControlEnabled) {
        Serial.println("LoRa command ignored - LoRa control disabled");
        return RESP_NACK;
    }

    String upperCommand = command;
    upperCommand.toUpperCase();

    // Log the command received
    Serial.printf("Processing LoRa command: %s (temp: %.1f°F)\n", command.c_str(), currentTemp);

    // Safety check for temperature
    if (currentTemp > SAFETY_MAX_TEMP && upperCommand.indexOf("ON") >= 0) {
        String safetyMsg = "Safety: Temperature " + String(currentTemp, 1) + "°F exceeds max " + String(SAFETY_MAX_TEMP, 1) + "°F";
        Serial.println(safetyMsg);
        return RESP_ERROR;
    }

    // Process the command
    if (upperCommand == CMD_STOVE_ON || upperCommand == "STOVE_ON") {
        // Clear automatic control temporarily and turn on
        bool wasManualOverride = manualOverride;
        manualOverride = true;
        String result = turnOn();
        
        if (result.indexOf("ON") >= 0) {
            Serial.println("LoRa command: Stove turned ON");
            return RESP_STOVE_ON;
        } else {
            manualOverride = wasManualOverride; // Restore previous state
            Serial.println("LoRa command: Failed to turn stove ON");
            return RESP_NACK;
        }
    } 
    else if (upperCommand == CMD_STOVE_OFF || upperCommand == "STOVE_OFF") {
        // Turn off and potentially clear manual override
        String result = turnOff();
        if (manualOverride) {
            manualOverride = false; // Clear manual override when turning off via LoRa
        }
        
        if (result.indexOf("OFF") >= 0) {
            Serial.println("LoRa command: Stove turned OFF");
            return RESP_STOVE_OFF;
        } else {
            Serial.println("LoRa command: Failed to turn stove OFF");
            return RESP_NACK;
        }
    }
    else if (upperCommand == CMD_STATUS_REQUEST || upperCommand == "STATUS") {
        String status = getStatus(currentTemp, 0); // Hour of week not critical for status
        Serial.printf("LoRa status request: %s\n", status.c_str());
        
        if (status.indexOf("ON") >= 0) {
            return RESP_STOVE_ON;
        } else {
            return RESP_STOVE_OFF;
        }
    }
    else if (upperCommand == CMD_PING) {
        Serial.println("LoRa ping received");
        return RESP_PONG;
    }
    else {
        Serial.printf("LoRa command: Unknown command '%s'\n", command.c_str());
        return RESP_UNKNOWN;
    }
}

RelayControl& Stove::getRelayControl()
{
    return relayControl;
}

