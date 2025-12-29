/**
 * @file stove_relay.hpp
 * @brief Stove relay control class for gas stove on/off control
 * @version 1.0.0
 * @date 2025-12-29
 */

#pragma once

#include <Arduino.h>

/**
 * @class StoveRelay
 * @brief Controls gas stove via digital pin (HIGH = ON, LOW = OFF)
 *
 * This class provides safe control of a gas stove through a digital output pin.
 * It includes safety features and state tracking.
 */
class StoveRelay
{
private:
    int controlPin;
    bool currentState;
    bool isInitialized;
    unsigned long lastStateChange;

    // Safety features
    const unsigned long MIN_STATE_CHANGE_INTERVAL = 2000; // 2 seconds minimum between changes

public:
    /**
     * @brief Constructor
     */
    StoveRelay();

    /**
     * @brief Destructor
     */
    ~StoveRelay();

    /**
     * @brief Initialize the stove relay control
     * @param pin Digital pin number to control (e.g., D10)
     * @return true if initialization successful
     */
    bool setup(int pin);

    /**
     * @brief Turn the stove ON (set pin HIGH)
     * @return true if state changed successfully
     */
    bool turnOn();

    /**
     * @brief Turn the stove OFF (set pin LOW)
     * @return true if state changed successfully
     */
    bool turnOff();

    /**
     * @brief Get current stove state
     * @return true if stove is ON, false if OFF
     */
    bool isOn() const;

    /**
     * @brief Get current state as string
     * @return "ON" or "OFF"
     */
    String getStateString() const;

    /**
     * @brief Get time since last state change
     * @return milliseconds since last change
     */
    unsigned long getTimeSinceLastChange() const;

    /**
     * @brief Force immediate state change (bypasses safety interval)
     * @param state true for ON, false for OFF
     * @return true if successful
     */
    bool forceState(bool state);

    /**
     * @brief Check if relay is properly initialized
     * @return true if initialized and ready
     */
    bool isReady() const;
};