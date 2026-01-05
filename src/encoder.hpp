/**
 * @file encoder.hpp
 * @brief M5Dial Encoder Class Header File
 * @version 0.2
 * @date 2025-12-16
 *
 * @Hardwares: M5Dial
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#pragma once

#include <M5Unified.h>

/**
 * @class Encoder
 * @brief Encoder class for M5Dial device
 *
 * This class provides functionality to handle encoder input, button presses,
 * and display updates for the M5Dial device.
 */
class Encoder
{
private:
    long oldPosition; // Store previous encoder position

public:
    /**
     * @brief Constructor
     * Initializes encoder with default values
     */
    Encoder();

    /**
     * @brief Destructor
     */
    ~Encoder();

    /**
     * @brief Setup encoder functionality
     * Initializes encoder display and shows test message
     */
    void setup();

    /**
     * @brief Get current encoder position
     * @return Current encoder position value
     */
    long getPosition();

    /**
     * @brief Check if encoder position has changed
     * @return true if position changed since last check
     */
    bool hasPositionChanged();
};

// Global instance for easy access
extern Encoder encoder;