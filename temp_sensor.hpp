/**
 * @file temp_sensor.hpp
 * @brief Temperature Sensor Class Header File
 * @version 1.0
 * @date 2025-12-17
 *
 * @Hardware: Grove - Temperature Sensor V1.2
 *           (https://www.seeedstudio.com/Grove-Temperature-Sensor.html)
 *           (https://wiki.seeedstudio.com/Grove-Temperature_Sensor_V1.2/)
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 */

#pragma once

#include <m5dial.h>

/**
 * @class TemperatureSensor
 * @brief Temperature sensor class for Grove Temperature Sensor V1.2
 *
 * This class provides functionality to read temperature from the Grove Temperature
 * Sensor V1.2 which uses a thermistor (NTC). The sensor converts temperature to
 * resistance, which is then converted to a voltage that can be read by an ADC pin.
 */
class TemperatureSensor
{
private:
    int sensorPin;         // ADC pin connected to temperature sensor
    static const float R0; // Thermistor resistance at 25°C (10kΩ)
    static const float B;  // Beta coefficient of the thermistor (4275)
    static const float T0; // Reference temperature in Kelvin (25°C = 298.15K)

    /**
     * @brief Read raw ADC value from sensor
     * @return Raw ADC reading (0-1023 for 10-bit ADC)
     */
    int readRawValue();

    /**
     * @brief Convert ADC reading to resistance
     * @param adcValue Raw ADC reading
     * @return Resistance in Ohms
     */
    float calculateResistance(int adcValue);

public:
    /**
     * @brief Constructor with custom pin
     * @param pin ADC pin number (default: 36 for ADC1_0)
     */
    // TemperatureSensor(int pin = 36);
    // Grove - Temperature Sensor connect to A0
    TemperatureSensor(int pin = 0); // A0 = 0?

    /**
     * @brief Destructor
     */
    ~TemperatureSensor();

    /**
     * @brief Initialize the temperature sensor
     */
    void setup();

    /**
     * @brief Read temperature from sensor
     * @return Temperature in Celsius
     */
    float readTemperature();

    /**
     * @brief Read temperature in Fahrenheit
     * @return Temperature in Fahrenheit
     */
    float readTemperatureFahrenheit();

    /**
     * @brief Read temperature in Kelvin
     * @return Temperature in Kelvin
     */
    float readTemperatureKelvin();

    /**
     * @brief Get the current sensor pin
     * @return Current ADC pin number
     */
    int getSensorPin() const;

    /**
     * @brief Set a new sensor pin
     * @param pin New ADC pin number
     */
    void setSensorPin(int pin);

    /**
     * @brief Check if sensor reading is valid
     * @param temperature Temperature reading to validate
     * @return true if reading is within reasonable range
     */
    bool isValidReading(float temperature) const;
};

// Global instance for easy access
extern TemperatureSensor tempSensor;