# M5Dial Thermo

A thermostat project for the M5Stack Dial using temperature sensing and LoRaWAN
communication.

## Hardware

- **[M5Dial](https://m5stack.com/products/m5dial)** - Main controller with
  built-in dial encoder and display
  - Uses M5StampS3 (ESP32-S3) as the controller
- **[Grove - Temperature Sensor V1.2](https://www.seeedstudio.com/Grove-Temperature-Sensor.html)** -
  For temperature measurement
- **[Grove-Wio-E5 Wireless Module](https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html)** -
  LoRaWAN communication (STM32WLE5JC with SX126x)

## Features

- Temperature monitoring and display
- Interactive dial control for setting thresholds
- LoRaWAN connectivity for remote monitoring
- Real-time clock functionality
- Touch interface support
- Sleep modes for power management

## Development Environment

- **Platform**: Arduino M5Stack Board Manager v2.0.7
- **IDE**: Arduino IDE / PlatformIO

## Project Structure

- `thermo.ino` - Main Arduino sketch
- `encoder.cpp/.hpp` - Dial encoder handling
- `temp_sensor.cpp/.hpp` - Temperature sensor interface
- `stove.cpp/.hpp` - Stove control logic
- `rtc.cpp/.hpp` - Real-time clock functionality
- `m5dial.cpp/.hpp.old` - Legacy M5Dial interface files

## Installation

1. Install the Arduino M5Stack Board Manager v2.0.7
2. Install required libraries (see dependency list in thermo.ino)
3. Connect the hardware components according to the Grove connector layout
4. Upload the sketch to your M5Dial

## Notes

- Initial implementation assumes I2C interface for temperature sensor and Wio-E5
  module
- These modules do not actually support I2C - interface may need revision
- Temperature sensor uses analog interface
- Wio-E5 uses UART/AT commands for LoRaWAN communication

# Security Configuration Setup

## WiFi Credentials Setup

To protect sensitive information like WiFi credentials, this project uses a
separate secrets file that is not committed to version control.

### Initial Setup

1. **Copy the template file:**

   ```bash
   cp secrets_template.h secrets.h
   ```

2. **Edit `secrets.h` with your actual credentials:**

   ```cpp
   #define DEFAULT_WIFI_SSID "YourActualWiFiName"
   #define DEFAULT_WIFI_PASSWORD "YourActualWiFiPassword"
   ```

3. **Verify `secrets.h` is in `.gitignore`:** The file `secrets.h` should
   already be listed in `.gitignore` to prevent accidental commits.

### Important Notes

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

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file
for details.

## Author

John Cornelison (john@vashonSoftware.com)  
Version 2.0.0 - December 2025
