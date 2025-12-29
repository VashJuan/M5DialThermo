# 🌡️ M5Dial Thermo

A smart thermostat project for the M5Stack Dial using precision temperature
sensing and LoRaWAN communication. 🏠🔥

## 📝 User Notes

- 📄 The base temperature and hourly temperature adjustments are configured in
  the easily edited file: **temps.csv**
- 🌡️ The temperature schedule varies from -15°F (nighttime) to 0°F (daytime
  comfort periods)
- ✏️ Edit temps.csv to customize your thermostat schedule without modifying code
- 🌍 **Timezone Configuration**: When WiFi/NTP is unavailable, the system uses a
  fallback timezone stored in temps.csv (see README_TIMEZONE.md for details)
- ⚠️ If temps.csv is not found, the system uses fallback defaults (68°F base
  temperature) from temps.csv

## 🔧 Hardware

- 🎛️ **[M5Dial](https://m5stack.com/products/m5dial)** - Main controller with
  built-in dial encoder and display
  - Uses M5StampS3 (ESP32-S3) as the controller
- 🌡️
  **[Adafruit MCP9808 Precision I2C Temperature Sensor](http://www.adafruit.com/products/1782)** -
  High-precision temperature measurement with power management
  - ±0.25°C accuracy, up to 0.0625°C resolution
  - Sleep/wake functionality for power savings
  - I2C interface (no analog pins required)
- 📡
  **[Grove-Wio-E5 Wireless Module](https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html)** -
  LoRaWAN communication
  - supporting both (G)FSK, BPSK, (G)MSK, and LoRa® modulations
  - supports LoRaWAN® Class A/B/C protocol and a wide frequency plan, including
    EU868/US915/AU915/AS923/KR920/IN865
  - connect to different LoRaWAN server platforms like TheThingsStack,
    Chirpstack, and others
  - this Wio-E5 LoRaWAN STM32WLE5JC module, integrates the ARM Cortex M4
    ultra-low-power MCU core and Wio SX126x
  - LoRa details:
    <https://www.seeedstudio.com/blog/2020/08/03/lorapedia-an-introduction-of-lora-and-lorawan-technology/>

## ✨ Features

- 🔍 **Smart Temperature Monitoring:** Periodic temperature polling with
  intelligent power management
- 🔋 **Power Saving Modes:** Multiple power levels with automatic CPU frequency
  scaling
- 💾 **Cached Temperature Display:** Shows recent readings with timestamps
  during power save periods
- 🎛️ **Interactive Dial Control:** Manual threshold adjustment via built-in
  encoder
- 📡 **LoRaWAN Connectivity:** Remote monitoring capabilities (Grove-Wio-E5)
- ⏰ **Real-time Clock Functionality:** NTP synchronization with timezone
  support
- 👆 **Touch Interface Support:** Responsive button handling for manual
  overrides
- 😴 **Adaptive Sleep Modes:** Context-aware power management for battery
  optimization
- 🌍 **True Geographic Detection:** Automatic timezone via IP geolocation API
- ✅ Automatic timezone conversion to ESP32 format
- ✅ Multi-layer fallback system for reliability
- ✅ No manual configuration needed in most cases
- ✅ Works globally regardless of device location

### Power Management Features

- **Periodic Temperature Polling:**
  - Active mode: Temperature checked every 5 seconds
  - Power save mode: Temperature checked every 2 minutes
  - Smart sensor wake/sleep cycles to minimize power consumption
- **Adaptive Power Modes:**
  - **Active Mode:** Full performance (80MHz CPU) during user interaction
  - **Power Save Mode:** Reduced performance (40MHz CPU) after 3 seconds idle
  - **Deep Power Save:** Extended sleep with cached temperature display
- **Temperature Sensor Management:**
  - MCP9808 sensor automatically sleeps between readings
  - Cached temperature values displayed during inactive periods
  - Intelligent wake-up scheduling for periodic monitoring

### 🎯 M5Dial thermostat should

- 🚀 Show a splash screen with "M5Dial Thermostat v 2.0.0"
- 🔧 Initialize the temperature sensor (MCP9808) with power management
- 📶 Connect to WiFi and sync time via NTP
- 📱 Display temperature readings and stove status with adaptive refresh rates
- 👆 Respond to button presses for manual stove override
- ⚡ Use interrupt-driven input for responsive UI
- 🔋 **Automatically manage power consumption:**
  - 😴 Enter power save mode after 3 seconds of inactivity
  - 📉 Reduce CPU frequency and display updates during idle periods
  - 📊 Poll temperature every 2 minutes during power save mode
  - 💾 Show cached temperature readings with timestamps
  - ⏰ Wake sensor only when needed for new readings

### 📝 Note

The warning about flash size (16MB vs 8MB available) is just a configuration
mismatch but doesn't prevent operation since the actual firmware (889KB) fits
comfortably in the available 8MB flash.

## 💻 Development Environment

- **Platform**: Arduino M5Stack Board Manager v2.0.7
- **IDE**: PlatformIO within Visual Studio Code, or Arduino IDE

## 📁 Project Structure

- 📋 `_thermo.cpp` - Main Arduino sketch with power management
- 🎛️ `encoder.cpp/.hpp` - Dial encoder handling
- 🌡️ `temp_sensor.cpp/.hpp` - Temperature sensor interface with caching
- 🔥 `stove.cpp/.hpp` - Stove control logic
- ⏰ `rtc.cpp/.hpp` - Real-time clock functionality
- 📱 `display.cpp/.hpp` - Display management with power-aware updates
- 📚 `doc/POWER_MANAGEMENT.md` - Detailed power saving documentation
- 📖 `doc/MCP9808_USAGE_EXAMPLE.md` - Temperature sensor usage guide

## 🛠️ Installation

1. 📦 Install the Arduino M5Stack Board Manager v2.0.7
2. 📚 Install required libraries (see dependency list in thermo.ino)
3. 🔌 Connect the hardware components according to the Grove connector layout
4. 🔒 **Configure WiFi credentials** (see Security Configuration Setup below)
5. Hold down button 0 on the back of the dial while connecting the USB cable to
   power the M5Dial on in "download code" mode.
6. 📁 **Upload filesystem data**: `pio run --target uploadfs` (required for
   temps.csv and timezone fallback)
7. 🚀 **Upload firmware**: `pio run --target upload`
8. Cycle the power (USB connector) off and back on to enter "Run Mode".
9. Optionally connect a serial monitor in your IDE if you wish to view debug
   messages.

### 🐠 PlatformIO Commands

- 📁 `pio run --target uploadfs` - Upload filesystem data (temps.csv, etc.) to
  device
- 🚀 `pio run --target upload` - Upload firmware to device
- 🛠️ `pio run` - Build project without uploading

**📝 Note**: The filesystem upload (`uploadfs`) must be done at least once
before first use, and again whenever you modify `temps.csv` or other data files.

## 📝 Notes

- ⚠️ Initial implementation assumes I2C interface for temperature sensor and
  Wio-E5 module
- 🔧 These modules do not actually support I2C - interface may need revision
- 🌡️ Temperature sensor uses analog interface
- 📡 Wio-E5 uses UART/AT commands for LoRaWAN communication

## 🔒 Security Configuration Setup

### 📶 WiFi Credentials Setup

To protect sensitive information like WiFi credentials, this project uses a
separate secrets file that is not committed to version control.

### 🚀 Initial Setup

1. **📋 Copy the template file:**

   ```bash
   cp secrets_template.h secrets.h
   ```

2. **✏️ Edit `secrets.h` with your actual credentials:**

   ```cpp
   #define DEFAULT_WIFI_SSID "YourActualWiFiName"
   #define DEFAULT_WIFI_PASSWORD "YourActualWiFiPassword"
   ```

3. **✅ Verify `secrets.h` is in `.gitignore`:** The file `secrets.h` should
   already be listed in `.gitignore` to prevent accidental commits.

### ⚠️ Important Notes

- ⚠️ **Never commit `secrets.h` to version control**
- ✅ Always use `secrets_template.h` as a reference for the required structure
- 🔒 Keep your actual credentials in `secrets.h` only
- 📝 Update `secrets_template.h` if you add new secret configuration options

### File Structure

- `secrets_template.h` - Template file (committed to Git)
- `secrets.h` - Your actual credentials (ignored by Git)
- `.gitignore` - Contains entry for `secrets.h`

### Adding New Secrets

If you need to add new sensitive configuration (API keys, MQTT credentials,
etc.):

1. Add the `#define` to both `secrets_template.h` and `secrets.h`
2. Use placeholder values in `secrets_template.h`
3. Use real values in `secrets.h`

### Troubleshooting

If you get compilation errors about missing `secrets.h`:

1. Make sure you copied `secrets_template.h` to `secrets.h`
2. Verify the file exists in the same directory as `rtc.cpp`
3. Check that your WiFi credentials are properly defined in `secrets.h`

### Current Library Dependencies

- M5Unified @ 0.1.17
- M5GFX @ 0.1.17
- Adafruit MCP9808 Library @ 2.0.2
- ArduinoJson @ 7.4.2
- HTTPClient @ 2.0.0
- SPIFFS @ 2.0.0
- FS @ 2.0.0
- WiFi @ 2.0.0
- Wire @ 2.0.0

## Additional Documentation

- **[Power Management Guide](doc/POWER_MANAGEMENT.md)** - Detailed information
  about power saving features, battery optimization, and energy consumption
- **[MCP9808 Sensor Usage](doc/MCP9808_USAGE_EXAMPLE.md)** - Complete guide for
  the precision temperature sensor including caching features
- **[Timezone Configuration](README_TIMEZONE.md)** - Geographic timezone
  detection and fallback systems
- **[NTP Troubleshooting](README_NTP_TROUBLESHOOTING.md)** - Network time
  synchronization debugging
- **[Build Instructions](README_BUILD.md)** - Compilation and upload procedures

## 🌡️ Temperature Configuration

### 🎛️ Customizing Temperature Schedule

The thermostat temperature settings are configured through the `temps.csv` file,
making it easy for users to customize without modifying code.

### 📋 Temperature Configuration File Structure

The `temps.csv` file contains:

1. **🌡️ Base Temperature**: The baseline target temperature in Fahrenheit
2. **⏰ Hourly Offsets**: Temperature adjustments for each hour of the day (1 AM
   to Midnight)

### ✏️ Editing Temperature Schedule

1. **📝 Open `temps.csv` in any text editor or spreadsheet program**
2. **🎯 Modify the base temperature**: Change the value after `BaseTemperature,`
3. **⏰ Adjust hourly offsets**: Modify the temperature offset values for each
   hour
   - ❄️ Negative values reduce temperature below base
   - 🔥 Positive values increase temperature above base
   - 📝 Example: If base is 68°F and hour 1 has -15.0 offset, target temperature
     at 1 AM will be 53°F

### Example Temperature Schedule

```csv
BaseTemperature,70.0

# Hour,Temperature Offset,Description
1,-12.0,1 AM - Sleep mode (58°F)
8,0.0,8 AM - Normal comfort (70°F)
22,-8.0,10 PM - Evening wind-down (62°F)
```

### 📍 File Location

- 💾 Place `temps.csv` on the SD card or in the device's file system
- ⚙️ The system will automatically load settings on startup
- 📎 If file is not found, defaults to 68°F base with standard schedule

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file
for details.

## 👨‍💻 Author

John Cornelison (<john@vashonSoftware.com>)  
Version 2.0.0 - December 2025 🎉
