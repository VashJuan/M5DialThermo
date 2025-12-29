/**
 * @file status_led.hpp
 * @brief Status LED control for visual feedback
 * @version 1.0.0
 * @date 2025-12-29
 */

#pragma once

#include <Arduino.h>

// Status LED states
enum LEDStatus
{
    STATUS_INITIALIZING, // Slow pulse
    STATUS_WAITING,      // Slow blink
    STATUS_RECEIVING,    // Fast blink
    STATUS_STOVE_ON,     // Solid ON
    STATUS_STOVE_OFF,    // Solid OFF (very dim)
    STATUS_TIMEOUT,      // Fast flash
    STATUS_ERROR         // SOS pattern
};

/**
 * @class StatusLED
 * @brief Controls status LED for visual system feedback
 */
class StatusLED
{
private:
    int ledPin;
    LEDStatus currentStatus;
    bool isInitialized;

    // Animation timing
    unsigned long lastUpdate;
    bool ledState;
    int animationStep;

    // Update intervals for different patterns
    void updatePattern();

public:
    /**
     * @brief Constructor
     */
    StatusLED();

    /**
     * @brief Destructor
     */
    ~StatusLED();

    /**
     * @brief Initialize the status LED
     * @param pin Digital pin for LED control
     * @return true if initialization successful
     */
    bool setup(int pin);

    /**
     * @brief Set the current status
     * @param status New status to display
     */
    void setStatus(LEDStatus status);

    /**
     * @brief Update LED animation (call in main loop)
     */
    void update();

    /**
     * @brief Get current status
     * @return Current LED status
     */
    LEDStatus getStatus() const;

    /**
     * @brief Turn LED on/off directly
     * @param state true for ON, false for OFF
     */
    void setLED(bool state);
};