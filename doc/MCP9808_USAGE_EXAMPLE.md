# MCP9808 Temperature Sensor Class Usage Example

## Overview

The temperature sensor class has been updated from Grove Temperature Sensor V1.2
(analog thermistor) to the Adafruit MCP9808 Precision I2C Temperature Sensor.
This provides much higher accuracy, precision, and additional features like
power management.

## Key Features

- **High Precision**: Up to 0.0625°C resolution (±0.25°C typical accuracy)
- **I2C Interface**: Uses standard I2C communication (no analog pins required)
- **Multiple Addresses**: Supports 8 different I2C addresses (0x18-0x1F)
- **Power Management**: Sleep/wake functionality for battery savings
- **Configurable Resolution**: 4 different resolution modes with different
  sampling times
- **Temperature Caching**: Stores last readings in memory for power-efficient
  access
- **Automatic Wake/Sleep**: Intelligent sensor state management
- **Periodic Polling**: Optimized for long-term monitoring with minimal power
  usage

## Hardware Connections

Connect the MCP9808 to M5Dial Port A:

- **Red**: 5V power
- **Black**: Ground
- **White**: SCL (G15 on M5Dial)
- **Yellow**: SDA (G13 on M5Dial)

## Basic Usage

```cpp
#include "temp_sensor.hpp"

// Global instance is already created in temp_sensor.cpp
// TemperatureSensor tempSensor(0x18, MCP9808_Resolution::RES_0_0625C);

void setup() {
    Serial.begin(115200);

    // Initialize the sensor
    if (!tempSensor.setup()) {
        Serial.println("Failed to initialize temperature sensor!");
        return;
    }

    Serial.println("Temperature sensor initialized successfully!");
}

void loop() {
    // Read temperature in different units
    float celsius = tempSensor.readTemperature();
    float fahrenheit = tempSensor.readTemperatureFahrenheit();
    float kelvin = tempSensor.readTemperatureKelvin();

    Serial.printf("Temperature: %.2f°C, %.2f°F, %.2fK\n",
                  celsius, fahrenheit, kelvin);

    delay(2000);
}
```

## Advanced Features

### Resolution Control

```cpp
// Change resolution (affects precision and measurement time)
tempSensor.setResolution(MCP9808_Resolution::RES_0_25C);
Serial.printf("Current resolution: %s\n", tempSensor.getResolutionString());
```

### Power Management

```cpp
// Put sensor to sleep to save power (~0.1μA consumption)
tempSensor.shutdown();
delay(5000);  // Do other work...

// Wake up sensor when needed (~200μA consumption)
tempSensor.wakeUp();
float temp = tempSensor.readTemperature();  // Sensor auto-wakes if needed
```

### Cached Temperature Access

```cpp
// Read new temperature and cache it
float currentTemp = tempSensor.readTemperatureFahrenheit();

// Later, get cached value without sensor read (power efficient)
float cachedTemp = tempSensor.getLastTemperatureF();
float cachedTempC = tempSensor.getLastTemperatureC();
unsigned long readTime = tempSensor.getLastReadTime();

// Check if cached reading is available
if (!isnan(cachedTemp)) {
    unsigned long age = (millis() - readTime) / 1000; // seconds since reading
    Serial.printf("Cached temperature: %.1f°F (%lus old)\n", cachedTemp, age);
}
```

### Periodic Monitoring Example

````cpp
void loop() {
    static unsigned long lastReading = 0;
    static const unsigned long POLL_INTERVAL = 2 * 60 * 1000; // 2 minutes

    unsigned long now = millis();

    // Take new reading every 2 minutes
    if (now - lastReading >= POLL_INTERVAL) {
        if (!tempSensor.getAwakeStatus()) {
            tempSensor.wakeUp();
            delay(10); // Allow stabilization
        }

        float temp = tempSensor.readTemperatureFahrenheit();
        lastReading = now;

        Serial.printf("New reading: %.1f°F\n", temp);

        // Put back to sleep for power savings
        tempSensor.shutdown();
    } else {
        // Use cached reading for display
        float cachedTemp = tempSensor.getLastTemperatureF();
        if (!isnan(cachedTemp)) {
            unsigned long age = (now - tempSensor.getLastReadTime()) / 1000;
            Serial.printf("Cached: %.1f°F (%lus ago)\n", cachedTemp, age);
        }
    }

    delay(1000);
}

### Multiple Sensors

```cpp
// Create multiple sensor instances with different I2C addresses
TemperatureSensor indoor(0x18, MCP9808_Resolution::RES_0_0625C);
TemperatureSensor outdoor(0x19, MCP9808_Resolution::RES_0_25C);

void setup() {
    indoor.setup();
    outdoor.setup();
}

void loop() {
    float indoorTemp = indoor.readTemperature();
    float outdoorTemp = outdoor.readTemperature();

    Serial.printf("Indoor: %.2f°C, Outdoor: %.2f°C\n",
                  indoorTemp, outdoorTemp);
    delay(1000);
}
````

### I2C Address Configuration

```cpp
// Available addresses: 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
// Set by hardware jumpers A2, A1, A0 on the MCP9808 board

// Change address at runtime (if you rewired the jumpers)
if (tempSensor.setI2CAddress(0x19)) {
    Serial.println("Successfully changed I2C address to 0x19");
} else {
    Serial.println("Failed to change I2C address");
}
```

## Resolution Modes

| Mode        | Resolution | Sample Time | Use Case                      |
| ----------- | ---------- | ----------- | ----------------------------- |
| RES_0_5C    | 0.5°C      | 30ms        | Fast readings, less precision |
| RES_0_25C   | 0.25°C     | 65ms        | Balanced speed/precision      |
| RES_0_125C  | 0.125°C    | 130ms       | Good precision                |
| RES_0_0625C | 0.0625°C   | 250ms       | Maximum precision (default)   |

## Migration from Grove Sensor

The old Grove Temperature Sensor methods are no longer available:

- ❌ `setSensorPin(pin)` - No longer needed (uses I2C)
- ❌ `getSensorPin()` - No longer needed
- ✅ `readTemperature()` - Still available (now caches result)
- ✅ `readTemperatureFahrenheit()` - Still available (now caches result)
- ✅ `readTemperatureKelvin()` - Still available
- ✅ `isValidReading(temp)` - Still available
- ✅ `setup()` - Now returns bool for success/failure
- ✨ `getLastTemperatureC()` - **New**: Get cached Celsius reading
- ✨ `getLastTemperatureF()` - **New**: Get cached Fahrenheit reading
- ✨ `getLastReadTime()` - **New**: Get timestamp of last sensor read
- ✨ `getAwakeStatus()` - **New**: Check if sensor is awake
- ✨ `wakeUp()`/`shutdown()` - **New**: Manual power management

## Error Handling

```cpp
if (!tempSensor.setup()) {
    Serial.println("Sensor initialization failed - check I2C connections");
    return;
}

float temp = tempSensor.readTemperature();
if (!tempSensor.isValidReading(temp)) {
    Serial.println("Invalid temperature reading");
    // Handle error...
}
```

## Dependencies

Make sure to install the Adafruit MCP9808 library:

- Library Manager → Search "Adafruit MCP9808" → Install
- Or: https://github.com/adafruit/Adafruit_MCP9808_Library
