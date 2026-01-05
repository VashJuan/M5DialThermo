/**
 * @file temp_sensor.hpp
 * @brief Temperature Sensor Class Header File
 * @version 2.0
 * @date 2025-12-20
 *
 * @Hardware: Adafruit MCP9808 Precision I2C Temperature Sensor
 *           (http://www.adafruit.com/products/1782)
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * Adafruit_MCP9808: https://github.com/adafruit/Adafruit_MCP9808_Library
 */

#pragma once

#include <M5Unified.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"

/**
 * @enum MCP9808_Resolution
 * @brief Resolution modes for MCP9808 temperature sensor
 */
enum class MCP9808_Resolution : uint8_t
{
    RES_0_5C = 0,   // 0.5°C resolution, 30 ms sample time
    RES_0_25C = 1,  // 0.25°C resolution, 65 ms sample time
    RES_0_125C = 2, // 0.125°C resolution, 130 ms sample time
    RES_0_0625C = 3 // 0.0625°C resolution, 250 ms sample time
};

/**
 * @class TemperatureSensor
 * @brief Temperature sensor class for Adafruit MCP9808 Precision I2C Temperature Sensor
 *
 * This class provides functionality to read temperature from the MCP9808 I2C
 * temperature sensor with high precision and low power consumption.
 * The sensor supports multiple I2C addresses and configurable resolution.
 */
class TemperatureSensor
{
private:
    Adafruit_MCP9808 mcp9808;      // MCP9808 sensor object
    uint8_t i2cAddress;            // I2C address of the sensor (0x18-0x1F)
    MCP9808_Resolution resolution; // Current resolution setting
    bool isAwake;                  // Power state tracking
    float lastTemperatureC;        // Cached temperature reading in Celsius
    float lastTemperatureF;        // Cached temperature reading in Fahrenheit
    unsigned long lastReadTime;    // Timestamp of last sensor read

    /**
     * @brief Convert resolution enum to resolution mode value
     * @param res Resolution enum value
     * @return Resolution mode for Adafruit library
     */
    uint8_t getResolutionMode(MCP9808_Resolution res) const;

public:
    /**
     * @brief Constructor with I2C address and resolution
     * @param address I2C address (0x18-0x1F, default: 0x18)
     * @param res Initial resolution (default: 0.0625°C)
     */
    TemperatureSensor(uint8_t address = 0x18, MCP9808_Resolution res = MCP9808_Resolution::RES_0_0625C);

    /**
     * @brief Destructor
     */
    ~TemperatureSensor();

    /**
     * @brief Initialize the temperature sensor
     * @return true if initialization successful, false otherwise
     */
    bool setup();

    /**
     * @brief Read temperature from sensor in Celsius
     * @return Temperature in Celsius
     */
    float readTemperature();

    /**
     * @brief Read temperature in Fahrenheit
     * @return Temperature in Fahrenheit
     */
    float readTemperatureFahrenheit();

    /**
     * @brief Get last cached temperature reading in Celsius (no sensor read)
     * @return Last temperature reading in Celsius
     */

    /**
     * @brief Set the sensor resolution
     * @param res New resolution setting
     */

    /**
     * @brief Get the current I2C address
     * @return Current I2C address
     */
    uint8_t getI2CAddress() const;

    /**
     * @brief Wake up the sensor from shutdown mode
     */
    void wakeUp();

    /**
     * @brief Put the sensor into shutdown mode for power saving
     */
    void shutdown();

    /**
     * @brief Check if sensor is awake
     * @return true if sensor is awake, false if in shutdown
     */
    bool getAwakeStatus() const;

    /**
     * @brief Check if sensor reading is valid
     * @param temperature Temperature reading to validate
     * @return true if reading is within reasonable range
     */
    bool isValidReading(float temperature) const;

    /**
     * @brief Get resolution as string for debugging
     * @return Resolution description string
     */
    const char *getResolutionString() const;
};

// Global instance for easy access
extern TemperatureSensor tempSensor;