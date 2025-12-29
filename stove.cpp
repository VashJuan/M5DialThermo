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
#include "stove.hpp"

// Global instance for easy access
Stove stove;

// Safety maximum temperature
const float Stove::SAFETY_MAX_TEMP = 82.0;

// Temperature schedule adjustments throughout the day (°F)
// Index 0 is unused, indices 1-24 represent hours 1-24 (1 AM to Midnight)
const float Stove::timeOffset[25] = {
    0.0,   // Index 0 - unused
    -15.0, -15.0, -15.0, -15.0, -12.0, -5.0,   // 1 AM to 6 AM - Night/Sleep
    -2.0,   0.0,   0.0,  -1.0,  -3.0,  -4.0,   // 7 AM to Noon - Morning/Work
    -3.0,  -3.0,  -3.0,  -2.0,   0.0,   0.0,   // 1 PM to 6 PM - Afternoon
    -1.0,  -4.0,  -7.0, -10.0, -12.0, -12.0    // 7 PM to Midnight - Evening/Night
};

Stove::Stove(int pin, float baseTemp) : 
    relayPin(pin), 
    currentState(STOVE_OFF), 
    baseTemperature(baseTemp),
    lastStateChange(0),
    minChangeInterval(300000), // 5 minutes in milliseconds
    enabled(true),
    manualOverride(false)
{
}

Stove::~Stove()
{
    // Turn off stove when object is destroyed
    setRelayState(false);
}

void Stove::setup()
{
    pinMode(relayPin, OUTPUT);
    setRelayState(false); // Start with stove off
    lastStateChange = millis();
    
    Serial.printf("Stove control initialized on pin %d, base temperature: %.1f°F\n", 
                  relayPin, baseTemperature);
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

    // If manual override is active, don't run automatic control
    if (manualOverride) {
        return (currentState == STOVE_ON) ? "ON" : "OFF";
    }

    if (!enabled) {
        Serial.println("Stove: not enabled");
        return "Stove: not enabled";
    }
        
    // Get desired temperature for current time
    float desiredTemp = getDesiredTemperature(hourOfWeek);
    
    // Calculate temperature difference
    float tempDiff = desiredTemp - currentTemp;
    if (loopCounter++ % 100 == 0) {
        Serial.printf("%lu) Stove: Current=%.1f°F, Desired=%.1f°F, Diff=%.1f°F, State=%s\n", 
                      loopCounter, currentTemp, desiredTemp, tempDiff, getStateString().c_str());
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

float Stove::getDesiredTemperature(int currentHour)
{    
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

unsigned long Stove::getTimeUntilNextChange()
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
        case STOVE_PENDING_ON:  return "PENDING_ON";
        case STOVE_PENDING_OFF: return "PENDING_OFF";
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

