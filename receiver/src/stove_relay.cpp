/**
 * @file stove_relay.cpp
 * @brief Stove relay control implementation
 * @version 1.0.0
 * @date 2025-12-29
 */

#include "stove_relay.hpp"

StoveRelay::StoveRelay() : controlPin(-1), currentState(false), isInitialized(false), lastStateChange(0) {
    // Constructor
}

StoveRelay::~StoveRelay() {
    // Ensure stove is turned off when object is destroyed
    if (isInitialized) {
        digitalWrite(controlPin, LOW);
    }
}

bool StoveRelay::setup(int pin) {
    controlPin = pin;
    
    Serial.printf("Setting up stove relay on pin %d\n", pin);
    
    // Configure pin as output
    pinMode(controlPin, OUTPUT);
    
    // Initialize to OFF state for safety
    digitalWrite(controlPin, LOW);
    currentState = false;
    lastStateChange = millis();
    
    // Verify pin configuration
    delay(100);
    int readBack = digitalRead(controlPin);
    if (readBack != LOW) {
        Serial.printf("Warning: Pin %d readback failed (expected LOW, got %d)\n", pin, readBack);
        return false;
    }
    
    isInitialized = true;
    Serial.printf("Stove relay initialized on pin %d (initial state: OFF)\n", pin);
    return true;
}

bool StoveRelay::turnOn() {
    if (!isInitialized) {
        Serial.println("Error: Stove relay not initialized");
        return false;
    }
    
    // Check safety interval
    if (millis() - lastStateChange < MIN_STATE_CHANGE_INTERVAL) {
        Serial.printf("Warning: State change too frequent (min interval: %lums)\n", MIN_STATE_CHANGE_INTERVAL);
        return false;
    }
    
    if (currentState) {
        Serial.println("Stove already ON");
        return true; // Already on
    }
    
    digitalWrite(controlPin, HIGH);
    currentState = true;
    lastStateChange = millis();
    
    // Verify state change
    delay(10);
    int readBack = digitalRead(controlPin);
    if (readBack != HIGH) {
        Serial.printf("Error: Failed to turn stove ON (pin readback: %d)\n", readBack);
        currentState = false;
        return false;
    }
    
    Serial.println("STOVE TURNED ON - Pin D10 set HIGH");
    return true;
}

bool StoveRelay::turnOff() {
    if (!isInitialized) {
        Serial.println("Error: Stove relay not initialized");
        return false;
    }
    
    // Check safety interval (but allow immediate OFF for safety)
    if (!currentState) {
        Serial.println("Stove already OFF");
        return true; // Already off
    }
    
    digitalWrite(controlPin, LOW);
    currentState = false;
    lastStateChange = millis();
    
    // Verify state change
    delay(10);
    int readBack = digitalRead(controlPin);
    if (readBack != LOW) {
        Serial.printf("Error: Failed to turn stove OFF (pin readback: %d)\n", readBack);
        return false;
    }
    
    Serial.println("STOVE TURNED OFF - Pin D10 set LOW");
    return true;
}

bool StoveRelay::isOn() const {
    return currentState;
}

String StoveRelay::getStateString() const {
    return currentState ? "ON" : "OFF";
}

unsigned long StoveRelay::getTimeSinceLastChange() const {
    return millis() - lastStateChange;
}

bool StoveRelay::forceState(bool state) {
    if (!isInitialized) {
        return false;
    }
    
    digitalWrite(controlPin, state ? HIGH : LOW);
    currentState = state;
    lastStateChange = millis();
    
    // Verify state change
    delay(10);
    int expected = state ? HIGH : LOW;
    int readBack = digitalRead(controlPin);
    if (readBack != expected) {
        Serial.printf("Error: Force state failed (expected %d, got %d)\n", expected, readBack);
        return false;
    }
    
    Serial.printf("STOVE FORCE STATE: %s - Pin D10 set %s\n", 
                  state ? "ON" : "OFF", 
                  state ? "HIGH" : "LOW");
    return true;
}

bool StoveRelay::isReady() const {
    return isInitialized && (controlPin >= 0);
}