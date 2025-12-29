/**
 * @file status_led.cpp
 * @brief Status LED implementation
 * @version 1.0.0
 * @date 2025-12-29
 */

#include "status_led.hpp"

StatusLED::StatusLED() : ledPin(-1), currentStatus(STATUS_INITIALIZING), 
                        isInitialized(false), lastUpdate(0), ledState(false), animationStep(0) {
    // Constructor
}

StatusLED::~StatusLED() {
    if (isInitialized) {
        digitalWrite(ledPin, LOW);
    }
}

bool StatusLED::setup(int pin) {
    ledPin = pin;
    
    Serial.printf("Setting up status LED on pin %d\n", pin);
    
    // Configure pin as output
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    isInitialized = true;
    lastUpdate = millis();
    
    Serial.printf("Status LED initialized on pin %d\n", pin);
    return true;
}

void StatusLED::setStatus(LEDStatus status) {
    if (currentStatus != status) {
        currentStatus = status;
        animationStep = 0;
        lastUpdate = millis();
        
        Serial.printf("Status LED changed to: ");
        switch (status) {
            case STATUS_INITIALIZING:
                Serial.println("INITIALIZING");
                break;
            case STATUS_WAITING:
                Serial.println("WAITING");
                break;
            case STATUS_RECEIVING:
                Serial.println("RECEIVING");
                break;
            case STATUS_STOVE_ON:
                Serial.println("STOVE_ON");
                break;
            case STATUS_STOVE_OFF:
                Serial.println("STOVE_OFF");
                break;
            case STATUS_TIMEOUT:
                Serial.println("TIMEOUT");
                break;
            case STATUS_ERROR:
                Serial.println("ERROR");
                break;
        }
    }
}

void StatusLED::update() {
    if (!isInitialized) {
        return;
    }
    
    updatePattern();
}

void StatusLED::updatePattern() {
    unsigned long currentTime = millis();
    unsigned long interval = 0;
    bool newLedState = ledState;
    
    switch (currentStatus) {
        case STATUS_INITIALIZING:
            // Slow pulse (2 second cycle)
            interval = 100;
            if (currentTime - lastUpdate >= interval) {
                animationStep++;
                if (animationStep >= 20) {
                    animationStep = 0;
                }
                // Sine wave approximation for smooth pulse
                newLedState = (animationStep < 10);
            }
            break;
            
        case STATUS_WAITING:
            // Slow blink (2 second cycle: 0.2s on, 1.8s off)
            interval = 2000;
            if (currentTime - lastUpdate >= interval) {
                animationStep = 0;
                lastUpdate = currentTime;
            }
            newLedState = (currentTime - lastUpdate) < 200;
            break;
            
        case STATUS_RECEIVING:
            // Fast blink (0.5 second cycle)
            interval = 250;
            if (currentTime - lastUpdate >= interval) {
                newLedState = !ledState;
                lastUpdate = currentTime;
            }
            break;
            
        case STATUS_STOVE_ON:
            // Solid ON
            newLedState = true;
            break;
            
        case STATUS_STOVE_OFF:
            // Solid OFF
            newLedState = false;
            break;
            
        case STATUS_TIMEOUT:
            // Fast flash (0.2 second cycle)
            interval = 100;
            if (currentTime - lastUpdate >= interval) {
                newLedState = !ledState;
                lastUpdate = currentTime;
            }
            break;
            
        case STATUS_ERROR:
            // SOS pattern: ... --- ... (dot=200ms, dash=600ms, space=200ms)
            interval = 200;
            if (currentTime - lastUpdate >= interval) {
                animationStep++;
                lastUpdate = currentTime;
                
                // SOS pattern timing
                if (animationStep <= 6) {
                    // Three dots: on-off-on-off-on-off
                    newLedState = (animationStep % 2 == 1);
                } else if (animationStep <= 7) {
                    // Space
                    newLedState = false;
                } else if (animationStep <= 13) {
                    // Three dashes: on(3x)-off-on(3x)-off-on(3x)-off
                    int dashStep = animationStep - 8;
                    if (dashStep == 0 || dashStep == 1 || dashStep == 2) newLedState = true;  // First dash
                    else if (dashStep == 3) newLedState = false;                             // Space
                    else if (dashStep == 4 || dashStep == 5 || dashStep == 6) newLedState = true;  // Second dash
                    else newLedState = false;                                                // Space before third dash
                    
                    // Adjust interval for dashes (3x longer)
                    interval = (dashStep % 4 < 3) ? 200 : 200;
                } else if (animationStep <= 14) {
                    // Space
                    newLedState = false;
                } else if (animationStep <= 20) {
                    // Three more dots
                    int dotStep = animationStep - 15;
                    newLedState = (dotStep % 2 == 0);
                } else {
                    // Long pause before repeat
                    animationStep = 0;
                    newLedState = false;
                    interval = 1000;
                }
            }
            break;
    }
    
    // Update LED if state changed
    if (newLedState != ledState && (currentTime - lastUpdate >= interval || currentStatus == STATUS_STOVE_ON || currentStatus == STATUS_STOVE_OFF)) {
        ledState = newLedState;
        digitalWrite(ledPin, ledState ? HIGH : LOW);
        
        if (currentStatus != STATUS_STOVE_ON && currentStatus != STATUS_STOVE_OFF) {
            lastUpdate = currentTime;
        }
    }
}

LEDStatus StatusLED::getStatus() const {
    return currentStatus;
}

void StatusLED::setLED(bool state) {
    if (isInitialized) {
        digitalWrite(ledPin, state ? HIGH : LOW);
        ledState = state;
    }
}