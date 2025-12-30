/**
 * @file relay_control.hpp
 * @brief Relay Control Class Header File - Extracted from Stove class
 * @version 1.0.0
 * @date 2025-12-30
 *
 * @Hardware: Generic relay control via GPIO pin
 * @Dependent Library: Arduino.h
 */

#pragma once

#include <Arduino.h>

/**
 * @enum RelayState
 * @brief Enumeration for relay states
 */
enum RelayState
{
    RELAY_OFF = 0,
    RELAY_ON = 1
};

/**
 * @class RelayControl
 * @brief Generic relay control class for GPIO-based relay switching
 *
 * This class provides functionality to:
 * 1. Control a relay via GPIO pin
 * 2. Track relay state
 * 3. Provide safety checks and timing controls
 * 4. Support both local and remote relay control
 */
class RelayControl
{
private:
    int relayPin;                    // GPIO pin for relay control
    RelayState currentState;         // Current relay state
    unsigned long lastStateChange;   // Time of last relay state change
    unsigned long minChangeInterval; // Minimum time between state changes
    bool enabled;                    // Whether relay control is enabled
    bool remoteControlEnabled;       // Whether remote control via LoRa is enabled
    String deviceName;               // Name for this relay device

    /**
     * @brief Set physical relay state
     * @param on true to turn relay on, false to turn off
     */
    void setPhysicalRelayState(bool on);

public:
    /**
     * @brief Constructor
     * @param pin GPIO pin for relay control (default: 2)
     * @param minInterval Minimum time between state changes in milliseconds (default: 3 minutes)
     * @param name Device name for identification (default: "Relay")
     */
    RelayControl(int pin = 2, unsigned long minInterval = 180000, const String &name = "Relay");

    /**
     * @brief Destructor
     */
    ~RelayControl();

    /**
     * @brief Initialize relay control system
     */
    void setup();

    /**
     * @brief Check if enough time has passed since last state change
     * @return true if state change is allowed
     */
    bool canChangeState() const;

    /**
     * @brief Turn relay on
     * @param force Force state change ignoring timing restrictions
     * @return Status message
     */
    String turnOn(bool force = false);

    /**
     * @brief Turn relay off
     * @param force Force state change ignoring timing restrictions
     * @return Status message
     */
    String turnOff(bool force = false);

    /**
     * @brief Get current relay state
     * @return Current RelayState
     */
    RelayState getState() const;

    /**
     * @brief Force immediate state change (ignores minimum interval)
     * Use with caution - intended for emergency situations
     * @param on true to turn on, false to turn off
     */
    void forceState(bool on);

    /**
     * @brief Enable or disable relay control
     * @param enable true to enable, false to disable
     */
    void setEnabled(bool enable);

    /**
     * @brief Check if relay control is enabled
     * @return true if enabled
     */
    bool isEnabled() const;

    /**
     * @brief Enable or disable remote control via LoRa
     * @param enable true to enable remote control, false to disable
     */
    void setRemoteControlEnabled(bool enable);

    /**
     * @brief Check if remote control is enabled
     * @return true if remote control is enabled
     */
    bool isRemoteControlEnabled() const;

    /**
     * @brief Get time remaining until next state change is allowed
     * @return Seconds remaining, 0 if change is allowed now
     */
    unsigned long getTimeUntilNextChange() const;

    /**
     * @brief Get string representation of current state
     * @return State as string
     */
    String getStateString() const;

    /**
     * @brief Get device name
     * @return Device name string
     */
    String getDeviceName() const;

    /**
     * @brief Set device name
     * @param name New device name
     */
    void setDeviceName(const String &name);

    /**
     * @brief Process remote control command
     * @param command Command string (e.g., "ON", "OFF", "STATUS")
     * @return Response string
     */
    String processRemoteCommand(const String &command);

    /**
     * @brief Set minimum change interval
     * @param intervalMs Minimum interval in milliseconds
     */
    void setMinChangeInterval(unsigned long intervalMs);

    /**
     * @brief Get minimum change interval
     * @return Minimum interval in milliseconds
     */
    unsigned long getMinChangeInterval() const;
};