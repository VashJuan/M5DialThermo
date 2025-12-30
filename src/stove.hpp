/**
 * @file stove.hpp
 * @brief Stove Control Class Header File
 * @version 1.0
 * @date 2025-12-17
 *
 * @Hardware: M5Dial with relay control for stove/heater
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#pragma once

#include <Arduino.h>
#include "temp_sensor.hpp"
#include "rtc.hpp"
#include "lora_transmitter.hpp"
#include "relay_control.hpp"

/**
 * @enum StoveState
 * @brief Enumeration for stove operating states
 */
enum StoveState
{
    STOVE_OFF = 0,
    STOVE_ON = 1,
    STOVE_PENDING_ON = 2,
    STOVE_PENDING_OFF = 3
};

// Turn on if temperature is 2°F or more below desired
static const float STOVE_HYSTERESIS_LOW = 2.0;
// Turn off if temperature is 0.5°F or more above desired
static const float STOVE_HYSTERESIS_HIGH = 0.5;

/**
 * @class Stove
 * @brief Stove control class for automated temperature management via LoRa
 *
 * This class provides functionality to:
 * 1. Read temperature schedule adjustments throughout the day
 * 2. Compare current room temperature with desired temperature
 * 3. Send LoRa commands to remote relay controller
 * 4. Display status received from remote controller
 * 5. Manage timing constraints and safety checks
 */
class Stove
{
private:
    LoRaTransmitter *loraTransmitter;   // LoRa transmitter instance (pointer for optional use)
    StoveState currentState;            // Current stove state (based on last known remote status)
    StoveState lastCommandedState;      // Last state we commanded via LoRa
    float baseTemperature;              // Base desired temperature (°F)
    unsigned long lastStateChange;      // Time of last state change command
    unsigned long lastStatusUpdate;     // Time of last status update from remote
    unsigned long minChangeInterval;    // Minimum time between state changes (3 minutes)
    bool enabled;                       // Whether automatic control is enabled
    bool manualOverride;                // Whether manual override is active
    bool loraControlEnabled;            // Whether LoRa remote control is enabled
    String lastLoRaResponse;            // Last response from LoRa transmitter
    String statusDisplayText;           // Current status text for display
    static const float SAFETY_MAX_TEMP; // Maximum safe temperature

    // Temperature schedule adjustments by hour (24-hour format)
    float timeOffset[25];

    /**
     * @brief Load configuration from temps.csv file
     * @return true if successful, false if file not found or error
     */
    bool loadConfigFromCSV();

    /**
     * @brief Get temperature adjustment for current time
     * @param hour Current hour (0-23)
     * @return Temperature adjustment in °F
     */
    float getTemperatureAdjustment(int hour);

    /**
     * @brief Check if enough time has passed since last state change
     * @return true if state change is allowed
     */
    bool canChangeState();

    /**
     * @brief Set physical relay state
     * @param on true to turn relay on, false to turn off
     */
    void setRelayState(bool on);

public:
    /**
     * @brief Constructor
     * @param transmitter Pointer to LoRa transmitter instance (optional, can be set later)
     * @param baseTemp Base desired temperature in °F (default: loaded from CSV, fallback: 68.0)
     */
    Stove(LoRaTransmitter *transmitter = nullptr, float baseTemp = -1.0);

    /**
     * @brief Destructor
     */
    ~Stove();

    /**
     * @brief Initialize stove control system
     */
    void setup();

    /**
     * @brief Update stove control based on current temperature and schedule
     * @param tempSensor Reference to temperature sensor
     * @param rtc Reference to RTC for time-based adjustments
     */
    String update(float currentTemp, int hourOfWeek);

    /**
     * @brief Manually turn stove on
     * Respects minimum change interval
     */
    String turnOn();

    /**
     * @brief Manually turn stove off
     * Respects minimum change interval
     */
    String turnOff();

    /**
     * @brief Get current stove state
     * @return Current StoveState
     */
    StoveState getState() const;

    /**
     * @brief Get current desired temperature for this time
     * @param rtc Reference to RTC for current time
     * @return Desired temperature in °F
     */
    float getDesiredTemperature(RTC &rtc);

    /**
     * @brief Get current desired temperature (using global rtc)
     * @return Desired temperature in °F
     */
    float getCurrentDesiredTemperature();

    /**
     * @brief Set base temperature
     * @param temp Base temperature in °F
     */
    void setBaseTemperature(float temp);

    /**
     * @brief Get base temperature
     * @return Base temperature in °F
     */
    float getBaseTemperature() const;

    /**
     * @brief Enable or disable automatic control
     * @param enable true to enable, false to disable
     */
    void setEnabled(bool enable);

    /**
     * @brief Check if automatic control is enabled
     * @return true if enabled
     */
    bool isEnabled() const;

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
     * @brief Force immediate state change (ignores minimum interval)
     * Use with caution - intended for emergency situations
     * @param on true to turn on, false to turn off
     */
    void forceState(bool on);

    /**
     * @brief Toggle manual override with safety check
     * @param currentTemp Current temperature for safety check
     * @return Status message indicating result of toggle attempt
     */
    String toggleManualOverride(float currentTemp);

    /**
     * @brief Check if manual override is active
     * @return true if manual override is active
     */
    bool isManualOverride() const;

    /**
     * @brief Get comprehensive status including manual override
     * @param currentTemp Current temperature
     * @param hourOfWeek Current hour of week for automatic mode
     * @return Status string (e.g., "MANUAL ON", "AUTO ON", "OFF", "OFF (Safety)")
     */
    String getStatus(float currentTemp, int hourOfWeek);

    /**
     * @brief Clear manual override and return to automatic mode
     */
    void clearManualOverride();

    /**
     * @brief Enable or disable LoRa remote control
     * @param enable true to enable LoRa control, false to disable
     */
    void setLoRaControlEnabled(bool enable);

    /**
     * @brief Check if LoRa remote control is enabled
     * @return true if LoRa control is enabled
     */
    bool isLoRaControlEnabled() const;

    /**
     * @brief Set LoRa transmitter instance
     * @param transmitter Pointer to LoRa transmitter instance
     */
    void setLoRaTransmitter(LoRaTransmitter *transmitter);

    /**
     * @brief Send command to remote stove via LoRa
     * @param command Command to send (STOVE_ON, STOVE_OFF, STATUS_REQUEST)
     * @return Response status for display
     */
    String sendLoRaCommand(const String &command);

    /**
     * @brief Update remote stove status via LoRa
     * @return Status string for display
     */
    String updateRemoteStatus();

    /**
     * @brief Get current status text for display
     * @return Status text including LoRa communication status
     */
    String getDisplayStatusText() const;

    /**
     * @brief Get last LoRa response
     * @return Last response from LoRa transmitter
     */
    String getLastLoRaResponse() const;

    /**
     * @brief Enable or disable LoRa remote control
     * @param enable true to enable LoRa control, false to disable
     */
    void setLoRaControlEnabled(bool enable);

    /**
     * @brief Check if LoRa remote control is enabled
     * @return true if LoRa control is enabled
     */
    bool isLoRaControlEnabled() const;

    /**
     * @brief Process LoRa remote control command
     * @param command Command string from LoRa (e.g., "STOVE_ON", "STOVE_OFF")
     * @param currentTemp Current temperature for safety checks
     * @return Response string to send back via LoRa
     */
    String processLoRaCommand(const String &command, float currentTemp);

    /**
     * @brief Get relay control instance (for direct access if needed)
     * @return Reference to RelayControl instance
     */
    RelayControl &getRelayControl();
};

// Declare a global instance of the Stove class
extern Stove stove;