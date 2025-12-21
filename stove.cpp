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
    enabled(true)
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

int Stove::getCurrentHour(RTC& rtc)
{
    if (!rtc.isSystemInitialized()) {
        return 12; // Default to noon if RTC not available
    }
    
    time_t currentTime = rtc.getCurrentTime();
    struct tm* timeInfo = localtime(&currentTime);
    
    // Convert to 1-24 hour format for array indexing
    int hour = timeInfo->tm_hour;
    return (hour == 0) ? 24 : hour;
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

String Stove::update(TemperatureSensor& tempSensor, RTC& rtc)
{
    String status = "";
    static unsigned long loopCounter = 0;

    if (!enabled) {
        Serial.println("Stove: not enabled");
        return "Stove: not enabled";
    }
    
    // Get current temperature
    float currentTemp = tempSensor.readTemperatureFahrenheit();
    
    if (!tempSensor.isValidReading(currentTemp)) {
        Serial.println("Stove: Invalid temperature reading, maintaining current state");
        return "Stove: Invalid temperature reading";
    }
    
    // Get desired temperature for current time
    float desiredTemp = getDesiredTemperature(rtc);
    
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

float Stove::getDesiredTemperature(RTC& rtc)
{
    int currentHour = getCurrentHour(rtc);
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

Stove stove;