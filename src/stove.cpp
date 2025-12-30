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
#include "lora_transmitter.hpp"
#include "../shared/protocol_common.hpp"

// Global instance for easy access
Stove stove;

// External reference to global RTC instance from main
extern RTC rtc;

// Safety maximum temperature
const float Stove::SAFETY_MAX_TEMP = 82.0;

Stove::Stove(LoRaTransmitter* transmitter, float baseTemp) : 
    loraTransmitter(transmitter),
    currentState(STOVE_OFF), 
    lastCommandedState(STOVE_OFF),
    lastStateChange(0),
    lastStatusUpdate(0),
    minChangeInterval(180000), // 3 minutes delay between state changes
    enabled(true),
    manualOverride(false),
    loraControlEnabled(false),
    lastLoRaResponse(""),
    statusDisplayText("LoRa: Not connected")
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
    currentState = STOVE_OFF;
    lastCommandedState = STOVE_OFF;
    lastStateChange = millis();
    lastStatusUpdate = 0;
    
    if (loraTransmitter && loraTransmitter->isReady()) {
        statusDisplayText = "LoRa: Ready";
        loraControlEnabled = true;
        Serial.println("Stove control initialized with LoRa transmitter");
    } else {
        statusDisplayText = "LoRa: Not available";
        loraControlEnabled = false;
        Serial.println("Stove control initialized without LoRa (local mode only)");
    }
    
    Serial.printf("Base temperature: %.1f°F\n", baseTemperature);
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

    // If manual override is active, don't run automatic control but do update status
    if (manualOverride) {
        if (!(loopCounter % 500)) {
            Serial.println("DEBUG: Manual override active, skipping automatic control");
        }
        
        // Update display with current manual state
        if (loraControlEnabled) {
            statusDisplayText = (currentState == STOVE_ON) ? "ON (Manual)" : "OFF (Manual)";
        }
        
        return getDisplayStatusText();
    }

    if (!enabled) {
        statusDisplayText = "Disabled";
        Serial.println("Stove: not enabled");
        return statusDisplayText;
    }

    // For LoRa control, we periodically check status and send commands
    if (loraControlEnabled && loraTransmitter) {
        // Update remote status periodically
        updateRemoteStatus();
        
        float desiredTemp = getDesiredTemperature(rtc);   
        float tempDiff = desiredTemp - currentTemp;
        
        if (!(loopCounter % 100)) {
            Serial.printf("%lu) Temp: Current=%.1f°F, Target=%.1f°F, Diff=%.1f°F, State=%s\n", 
                          loopCounter, currentTemp, desiredTemp, tempDiff, getStateString().c_str());
        }
        
        bool shouldBeOn = false;
        
        // Hysteresis: different thresholds for turning on vs off to prevent oscillation
        if (currentState == STOVE_OFF || currentState == STOVE_PENDING_OFF) {
            shouldBeOn = (tempDiff >= STOVE_HYSTERESIS_LOW);
        } else {
            shouldBeOn = (tempDiff > STOVE_HYSTERESIS_HIGH);
        }
        
        // Send command if needed and timing allows
        if (shouldBeOn && currentState == STOVE_OFF && canChangeState()) {
            status = turnOn();
        } else if (!shouldBeOn && currentState == STOVE_ON && canChangeState()) {
            status = turnOff();
        }
        
        // Update display text with temperature info
        if (currentState == STOVE_PENDING_ON || currentState == STOVE_PENDING_OFF) {
            unsigned long remainingSeconds = getTimeUntilNextChange();
            statusDisplayText = getStateString() + " (" + String(remainingSeconds) + "s)";
        }
    } else {
        // No LoRa control - just show local calculation
        float desiredTemp = getDesiredTemperature(rtc);   
        float tempDiff = desiredTemp - currentTemp;
        
        statusDisplayText = "Local mode - Target: " + String(desiredTemp, 1) + "°F";
        
        if (!(loopCounter % 100)) {
            Serial.printf("Local mode: Current=%.1f°F, Target=%.1f°F, Diff=%.1f°F\n", 
                          currentTemp, desiredTemp, tempDiff);
        }
    }
    
    loopCounter++;
    return getDisplayStatusText();
}

String Stove::turnOn()
{
    if (!loraControlEnabled || !loraTransmitter) {
        statusDisplayText = "LoRa: Not available";
        return "LoRa not available";
    }
    
    if (!canChangeState() && lastCommandedState != STOVE_ON) {
        unsigned long remainingSeconds = getTimeUntilNextChange();
        statusDisplayText = "Wait " + String(remainingSeconds) + "s";
        return statusDisplayText;
    }
    
    statusDisplayText = "Sending ON command...";
    String response = sendLoRaCommand(CMD_STOVE_ON);
    
    if (response == RESP_STOVE_ON) {
        currentState = STOVE_ON;
        lastCommandedState = STOVE_ON;
        lastStateChange = millis();
        statusDisplayText = "ON (LoRa)";
        Serial.println("Stove: Remote turned ON");
        return "Stove: Remote turned ON";
    } else {
        statusDisplayText = "ON Failed: " + response;
        Serial.println("Stove: Failed to turn ON - " + response);
        return "Failed: " + response;
    }
}

String Stove::turnOff()
{
    if (!loraControlEnabled || !loraTransmitter) {
        statusDisplayText = "LoRa: Not available";
        return "LoRa not available";
    }
    
    if (!canChangeState() && lastCommandedState != STOVE_OFF) {
        unsigned long remainingSeconds = getTimeUntilNextChange();
        statusDisplayText = "Wait " + String(remainingSeconds) + "s";
        return statusDisplayText;
    }
    
    statusDisplayText = "Sending OFF command...";
    String response = sendLoRaCommand(CMD_STOVE_OFF);
    
    if (response == RESP_STOVE_OFF) {
        currentState = STOVE_OFF;
        lastCommandedState = STOVE_OFF;
        lastStateChange = millis();
        statusDisplayText = "OFF (LoRa)";
        Serial.println("Stove: Remote turned OFF");
        return "Stove: Remote turned OFF";
    } else {
        statusDisplayText = "OFF Failed: " + response;
        Serial.println("Stove: Failed to turn OFF - " + response);
        return "Failed: " + response;
    }
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
    Serial.printf("Stove: LoRa remote control %s\n", enable ? "ENABLED" : "DISABLED");
    
    if (enable && loraTransmitter && loraTransmitter->isReady()) {
        statusDisplayText = "LoRa: Ready";
    } else if (enable) {
        statusDisplayText = "LoRa: Not available";
    } else {
        statusDisplayText = "LoRa: Disabled";
    }
}

void Stove::setLoRaTransmitter(LoRaTransmitter* transmitter)
{
    loraTransmitter = transmitter;
    
    if (transmitter && transmitter->isReady()) {
        statusDisplayText = "LoRa: Connected";
        Serial.println("LoRa transmitter connected and ready");
    } else {
        statusDisplayText = "LoRa: Not available";
        Serial.println("LoRa transmitter not available");
    }
}

String Stove::sendLoRaCommand(const String& command)
{
    if (!loraTransmitter) {
        lastLoRaResponse = "No transmitter";
        return lastLoRaResponse;
    }
    
    if (!loraTransmitter->isReady()) {
        lastLoRaResponse = "Transmitter not ready";
        return lastLoRaResponse;
    }
    
    Serial.printf("Sending LoRa command: %s\n", command.c_str());
    statusDisplayText = "Sending: " + command;
    
    String response = loraTransmitter->sendCommand(command, LORAWAN_PORT_CONTROL, true, 2);
    lastLoRaResponse = response;
    lastStatusUpdate = millis();
    
    if (response.length() == 0) {
        statusDisplayText = "No response";
        return "TIMEOUT";
    }
    
    return response;
}

String Stove::updateRemoteStatus()
{
    if (!loraControlEnabled || !loraTransmitter) {
        statusDisplayText = "LoRa: Not available";
        return statusDisplayText;
    }
    
    // Only update status if it's been a while since last update
    if (millis() - lastStatusUpdate > 30000) { // 30 seconds
        statusDisplayText = "Getting status...";
        String response = sendLoRaCommand(CMD_STATUS_REQUEST);
        
        if (response == RESP_STOVE_ON) {
            currentState = STOVE_ON;
            statusDisplayText = "ON (Remote)";
        } else if (response == RESP_STOVE_OFF) {
            currentState = STOVE_OFF;
            statusDisplayText = "OFF (Remote)";
        } else if (response == "TIMEOUT") {
            statusDisplayText = "LoRa: No response";
        } else {
            statusDisplayText = "LoRa: " + response;
        }
    }
    
    return statusDisplayText;
}

String Stove::getDisplayStatusText() const
{
    return statusDisplayText;
}

String Stove::getLastLoRaResponse() const
{
    return lastLoRaResponse;
}

bool Stove::isLoRaControlEnabled() const
{
    return loraControlEnabled;
}

