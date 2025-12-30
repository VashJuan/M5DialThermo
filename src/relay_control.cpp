/**
 * @file relay_control.cpp
 * @brief Relay Control Class Implementation - Extracted from Stove class
 * @version 1.0.0
 * @date 2025-12-30
 */

#include "relay_control.hpp"

RelayControl::RelayControl(int pin, unsigned long minInterval, const String& name) : 
    relayPin(pin), 
    currentState(RELAY_OFF), 
    lastStateChange(0),
    minChangeInterval(minInterval),
    enabled(true),
    remoteControlEnabled(false),
    deviceName(name)
{
    // Constructor initialization
}

RelayControl::~RelayControl()
{
    // Turn off relay when object is destroyed
    setPhysicalRelayState(false);
}

void RelayControl::setup()
{
    pinMode(relayPin, OUTPUT);
    setPhysicalRelayState(false); // Start with relay off
    lastStateChange = millis();
    
    Serial.printf("Relay control '%s' initialized on pin %d\n", 
                  deviceName.c_str(), relayPin);
    Serial.printf("Minimum change interval: %lu seconds\n", minChangeInterval / 1000);
}

bool RelayControl::canChangeState() const
{
    return (millis() - lastStateChange) >= minChangeInterval;
}

void RelayControl::setPhysicalRelayState(bool on)
{
    digitalWrite(relayPin, on ? HIGH : LOW);
    Serial.printf("%s relay set to: %s\n", deviceName.c_str(), on ? "ON" : "OFF");
}

String RelayControl::turnOn(bool force)
{
    if (!enabled) {
        String msg = deviceName + ": Control disabled";
        Serial.println(msg);
        return msg;
    }

    if (!force && !canChangeState()) {
        unsigned long remainingSeconds = getTimeUntilNextChange();
        String msg = deviceName + ": Cannot turn on, " + String(remainingSeconds) + " seconds remaining";
        Serial.println(msg);
        return msg;
    }
    
    currentState = RELAY_ON;
    lastStateChange = millis();
    setPhysicalRelayState(true);
    
    String msg = deviceName + ": Turned ON";
    Serial.println(msg);
    return msg;
}

String RelayControl::turnOff(bool force)
{
    if (!enabled) {
        String msg = deviceName + ": Control disabled";
        Serial.println(msg);
        return msg;
    }

    if (!force && !canChangeState()) {
        unsigned long remainingSeconds = getTimeUntilNextChange();
        String msg = deviceName + ": Cannot turn off, " + String(remainingSeconds) + " seconds remaining";
        Serial.println(msg);
        return msg;
    }
    
    currentState = RELAY_OFF;
    lastStateChange = millis();
    setPhysicalRelayState(false);
    
    String msg = deviceName + ": Turned OFF";
    Serial.println(msg);
    return msg;
}

RelayState RelayControl::getState() const
{
    return currentState;
}

void RelayControl::forceState(bool on)
{
    Serial.printf("%s: FORCE state to %s\n", deviceName.c_str(), on ? "ON" : "OFF");
    
    currentState = on ? RELAY_ON : RELAY_OFF;
    lastStateChange = millis();
    setPhysicalRelayState(on);
}

void RelayControl::setEnabled(bool enable)
{
    enabled = enable;
    Serial.printf("%s: Control %s\n", deviceName.c_str(), enable ? "ENABLED" : "DISABLED");
    
    if (!enable && (currentState == RELAY_ON)) {
        // Turn off relay when disabling control
        forceState(false);
    }
}

bool RelayControl::isEnabled() const
{
    return enabled;
}

void RelayControl::setRemoteControlEnabled(bool enable)
{
    remoteControlEnabled = enable;
    Serial.printf("%s: Remote control %s\n", deviceName.c_str(), enable ? "ENABLED" : "DISABLED");
}

bool RelayControl::isRemoteControlEnabled() const
{
    return remoteControlEnabled;
}

unsigned long RelayControl::getTimeUntilNextChange() const
{
    unsigned long elapsed = millis() - lastStateChange;
    if (elapsed >= minChangeInterval) {
        return 0;
    }
    return (minChangeInterval - elapsed) / 1000; // Return in seconds
}

String RelayControl::getStateString() const
{
    String baseState = (currentState == RELAY_ON) ? "ON" : "OFF";
    
    if (!enabled) {
        return baseState + " (Disabled)";
    }
    
    if (!canChangeState()) {
        unsigned long remainingSeconds = getTimeUntilNextChange();
        return baseState + " (Change in " + String(remainingSeconds) + "s)";
    }
    
    return baseState;
}

String RelayControl::getDeviceName() const
{
    return deviceName;
}

void RelayControl::setDeviceName(const String& name)
{
    deviceName = name;
}

String RelayControl::processRemoteCommand(const String& command)
{
    if (!remoteControlEnabled) {
        return "Remote control disabled";
    }

    if (!enabled) {
        return "Control disabled";
    }

    String upperCommand = command;
    upperCommand.toUpperCase();

    if (upperCommand == "ON" || upperCommand == "STOVE_ON") {
        return turnOn();
    } else if (upperCommand == "OFF" || upperCommand == "STOVE_OFF") {
        return turnOff();
    } else if (upperCommand == "STATUS" || upperCommand == "STATUS_REQUEST") {
        return deviceName + ": " + getStateString();
    } else if (upperCommand == "FORCE_ON") {
        return turnOn(true);
    } else if (upperCommand == "FORCE_OFF") {
        return turnOff(true);
    } else {
        return "Unknown command: " + command;
    }
}

void RelayControl::setMinChangeInterval(unsigned long intervalMs)
{
    minChangeInterval = intervalMs;
    Serial.printf("%s: Minimum change interval set to %lu seconds\n", 
                  deviceName.c_str(), intervalMs / 1000);
}

unsigned long RelayControl::getMinChangeInterval() const
{
    return minChangeInterval;
}