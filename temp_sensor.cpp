/**
 * @file temp_sensor.cpp
 * @brief Temperature Sensor Class Implementation
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

#include "M5Dial.h"
#include "temp_sensor.hpp"

TemperatureSensor::TemperatureSensor(uint8_t address, MCP9808_Resolution res) 
    : i2cAddress(address), resolution(res), isAwake(false)
{
}

TemperatureSensor::~TemperatureSensor()
{
    // Put sensor to sleep before destruction to save power
    if (isAwake) {
        shutdown();
    }
}

uint8_t TemperatureSensor::getResolutionMode(MCP9808_Resolution res) const
{
    return static_cast<uint8_t>(res);
}

bool TemperatureSensor::setup()
{
    // Initialize I2C if not already done
    Wire.begin();
    
    // Initialize the sensor with the specified I2C address
    if (!mcp9808.begin(i2cAddress)) {
        Serial.printf("Couldn't find MCP9808 temperature sensor at address 0x%02X! Check connections.\n", i2cAddress);
        return false;
    }
    
    Serial.printf("Found MCP9808 temperature sensor at address 0x%02X!\n", i2cAddress);
    
    // Set the initial resolution
    mcp9808.setResolution(getResolutionMode(resolution));
    Serial.printf("Resolution set to mode %d (%s)\n", static_cast<int>(resolution), getResolutionString());
    
    // Wake up the sensor
    wakeUp();
    
    return true;
}

float TemperatureSensor::readTemperature()
{
    if (!isAwake) {
        wakeUp();
        // Allow some time for the sensor to wake up and stabilize
        delay(10);
    }
    
    // Read temperature in Celsius
    float temperature = mcp9808.readTempC();
    
    // Check if the reading is valid
    if (isnan(temperature) || !isValidReading(temperature)) {
        Serial.println("Invalid temperature reading!");
        return NAN;
    }
    
    return temperature;
}

float TemperatureSensor::readTemperatureFahrenheit()
{
    if (!isAwake) {
        wakeUp();
        delay(10);
    }
    
    // Read temperature in Fahrenheit directly from the sensor
    float temperature = mcp9808.readTempF();
    
    if (isnan(temperature)) {
        Serial.println("Invalid temperature reading");
        return NAN;
    }
    
    return temperature;
}

float TemperatureSensor::readTemperatureKelvin()
{
    float celsius = readTemperature();
    if (isnan(celsius)) {
        return NAN;
    }
    return celsius + 273.15;
}

void TemperatureSensor::setResolution(MCP9808_Resolution res)
{
    resolution = res;
    mcp9808.setResolution(getResolutionMode(res));
    Serial.printf("Resolution changed to mode %d (%s)\n", static_cast<int>(res), getResolutionString());
}

MCP9808_Resolution TemperatureSensor::getResolution() const
{
    return resolution;
}

uint8_t TemperatureSensor::getI2CAddress() const
{
    return i2cAddress;
}

bool TemperatureSensor::setI2CAddress(uint8_t address)
{
    // Validate address range (0x18-0x1F are valid for MCP9808)
    if (address < 0x18 || address > 0x1F) {
        Serial.printf("Invalid I2C address 0x%02X! Valid range: 0x18-0x1F\n", address);
        return false;
    }
    
    // Save current state
    bool wasAwake = isAwake;
    
    // Shutdown current sensor
    if (isAwake) {
        shutdown();
    }
    
    // Update address
    i2cAddress = address;
    
    // Try to initialize with new address
    if (!mcp9808.begin(i2cAddress)) {
        Serial.printf("Failed to initialize MCP9808 at new address 0x%02X\n", i2cAddress);
        return false;
    }
    
    // Restore previous settings
    mcp9808.setResolution(getResolutionMode(resolution));
    
    if (wasAwake) {
        wakeUp();
    }
    
    Serial.printf("Successfully changed I2C address to 0x%02X\n", i2cAddress);
    return true;
}

void TemperatureSensor::wakeUp()
{
    mcp9808.wake();
    isAwake = true;
    Serial.println("MCP9808 sensor woken up - ready to read!");
}

void TemperatureSensor::shutdown()
{
    mcp9808.shutdown_wake(1); // 1 = shutdown mode
    isAwake = false;
    Serial.println("MCP9808 sensor shutdown - low power mode");
}

bool TemperatureSensor::getAwakeStatus() const
{
    return isAwake;
}

bool TemperatureSensor::isValidReading(float temperature) const
{
    // Check if temperature is within reasonable range for MCP9808 sensor
    // MCP9808 can measure -40°C to 125°C, but we'll use a more conservative range
    return (!isnan(temperature) && temperature >= -40.0 && temperature <= 125.0);
}

const char* TemperatureSensor::getResolutionString() const
{
    switch (resolution) {
        case MCP9808_Resolution::RES_0_5C:
            return "0.5°C (30ms)";
        case MCP9808_Resolution::RES_0_25C:
            return "0.25°C (65ms)";
        case MCP9808_Resolution::RES_0_125C:
            return "0.125°C (130ms)";
        case MCP9808_Resolution::RES_0_0625C:
            return "0.0625°C (250ms)";
        default:
            return "Unknown";
    }
}

// Global instance for easy access (using default I2C address 0x18 and highest resolution)
TemperatureSensor tempSensor(0x18, MCP9808_Resolution::RES_0_0625C);