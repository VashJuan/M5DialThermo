/**
 * @file temp_sensor.cpp
 * @brief Temperature Sensor Class Implementation
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

#include "m5dial.h"
#include "temp_sensor.hpp"
#include <math.h>

// Static constants for Grove Temperature Sensor V1.2 thermistor
const float TemperatureSensor::R0 = 10000.0;    // 10kΩ at 25°C
const float TemperatureSensor::B = 4275.0;      // Beta coefficient
const float TemperatureSensor::T0 = 298.15;     // 25°C in Kelvin

TemperatureSensor::TemperatureSensor(int pin) : sensorPin(pin)
{
}

TemperatureSensor::~TemperatureSensor()
{
    // No cleanup needed
}

void TemperatureSensor::setup()
{
    // ADC pins don't require explicit initialization on ESP32
    // but we can add any future initialization code here
    Serial.printf("Temperature sensor initialized on pin %d\n", sensorPin);
}

int TemperatureSensor::readRawValue()
{
    return analogRead(sensorPin);
}

float TemperatureSensor::calculateResistance(int adcValue)
{
    // Convert ADC reading to resistance using voltage divider formula
    // Grove sensor uses a 10kΩ pull-up resistor
    if (adcValue == 0) {
        return R0 * 1000; // Avoid division by zero, return very high resistance
    }
    
    float resistance = 1023.0 / adcValue - 1.0;
    resistance = R0 * resistance;
    return resistance;
}

float TemperatureSensor::readTemperature()
{
    int adcValue = readRawValue();
    float resistance = calculateResistance(adcValue);
    
    // Use Steinhart-Hart equation (simplified beta formula)
    // 1/T = 1/T0 + (1/B) * ln(R/R0)
    // T = 1 / (1/T0 + (1/B) * ln(R/R0))
    float temperatureKelvin = 1.0 / (log(resistance / R0) / B + 1.0 / T0);
    
    // Convert from Kelvin to Celsius
    float temperatureCelsius = temperatureKelvin - 273.15;
    
    return temperatureCelsius;
}

float TemperatureSensor::readTemperatureFahrenheit()
{
    float celsius = readTemperature();
    return (celsius * 9.0 / 5.0) + 32.0;
}

float TemperatureSensor::readTemperatureKelvin()
{
    float celsius = readTemperature();
    return celsius + 273.15;
}

int TemperatureSensor::getSensorPin() const
{
    return sensorPin;
}

void TemperatureSensor::setSensorPin(int pin)
{
    sensorPin = pin;
}

bool TemperatureSensor::isValidReading(float temperature) const
{
    // Check if temperature is within reasonable range for typical environments
    // -40°C to 85°C is a reasonable range for this sensor
    return (temperature >= -40.0 && temperature <= 85.0);
}

// Global instance for easy access
TemperatureSensor tempSensor;