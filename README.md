# 🌡️ M5Dial Smart Thermostat

A smart thermostat system for the M5Stack Dial using precision temperature
sensing and LoRaWAN communication. 🏠🔥

## 📁 Project Structure

This project consists of two main components:

### 🎛️ **Main Thermostat** (`/src/`)

- **M5Stack Dial** - Display, controls, and temperature sensing
- **MCP9808 Temperature Sensor** - Precision temperature measurement
- **Grove-Wio-E5** - LoRaWAN transmitter for stove control

### 📡 **Receiver/Relay** (`/receiver/`)

- **XIAO ESP32S3** - Relay controller
- **Grove-Wio-E5** - LoRaWAN receiver
- **Pin D10 Control** - Gas stove ON/OFF control (HIGH/LOW)

### 🤝 **Shared Components** (`/shared/`)

- **Communication Protocol** - LoRaWAN message definitions
- **Common Utilities** - Shared code between devices

## 🌟 Features

### 🌡️ Temperature Control

- **Precision Monitoring**: MCP9808 sensor with ±0.25°C accuracy
- **Smart Caching**: Temperature readings cached for optimal battery life
- **Configurable Schedules**: Easy temperature customization via temps.csv file
- **Manual Override**: Dial control for instant temperature adjustments

### ⏱️ Time & Scheduling

- **Real-Time Clock**: Integrated RTC with NTP synchronization
- **Automatic Timezone**: Smart geographic timezone detection with fallbacks
- **Temperature Logging**: Historical data storage for energy usage insights
- **Customizable Schedule**: 24-hour temperature profile configuration

### 🏠 Heating System Integration

- **Smart Stove Control**: Automated gas heating system on/off control
- **Safety Features**: Automatic shutoff mechanisms and temperature monitoring
- **Energy Efficiency**: Intelligent heating cycles to minimize energy usage
- **Remote Control**: LoRaWAN communication for wireless relay operation

### 🔋 Power Management

- **Battery Optimization**: Advanced power saving with multiple sleep modes
- **Auto Display Dimming**: Screen brightness adjusts to conserve power
- **Adaptive Power Modes**: CPU frequency scaling based on activity
- **Smart Sensor Wake**: Temperature sensor sleeps between readings

### 📱 User Interface

- **Rotary Encoder**: Smooth dial control for temperature adjustments
- **High-Resolution Display**: Clear LCD showing temperature, time, and status
- **Touch Controls**: Responsive button handling for manual overrides
- **Status Indicators**: Visual feedback for heating, connectivity, and power

## 🛠️ Hardware Requirements

### Required Components

- **M5Stack Dial** (ESP32-S3 based controller with built-in display and encoder)
- **MCP9808 Temperature Sensor** (high precision I2C sensor)
- **XIAO ESP32S3** (for receiver/relay unit)
- **2x Grove-Wio-E5 Modules** (LoRaWAN communication)
- **Relay Module** (for heating system control)

### Optional Components

- **External Battery Pack** (for extended operation)
- **Mounting Hardware** (for permanent installation)

## 🚀 Getting Started

### Basic Setup

1. **Assemble Hardware**: Connect components according to documentation
2. **Install Firmware**: Upload firmware to both thermostat and receiver
3. **Configure WiFi**: Set up network credentials for NTP time sync
4. **Customize Schedule**: Edit temps.csv for your temperature preferences

### Initial Installation

1. **Upload Data Files**: Use filesystem upload for temps.csv and configuration
2. **Configure Secrets**: Copy secrets template and add your WiFi credentials
3. **Test System**: Verify temperature reading and heating control operation
4. **Mount Devices**: Install in desired locations with good signal coverage

## 🌡️ Temperature Configuration

### 📋 Easy Schedule Customization

Temperature settings are configured through the `temps.csv` file - no
programming required!

### 📝 Editing Your Temperature Schedule

1. **Open `temps.csv`** in any text editor or Excel
2. **Set Base Temperature**: Modify the baseline temperature (e.g., 68°F)
3. **Adjust Hourly Offsets**: Customize temperature changes throughout the day
   - ❄️ Negative values = cooler (sleep periods)
   - 🔥 Positive values = warmer (comfort periods)
   - 📝 Example: Base 68°F + (-15°F at 1AM) = 53°F nighttime temperature

### Example Schedule

```csv
BaseTemperature,70.0

# Hour,Temperature Offset,Description
1,-12.0,1 AM - Sleep mode (58°F)
8,0.0,8 AM - Normal comfort (70°F)
17,2.0,5 PM - Evening comfort (72°F)
22,-8.0,10 PM - Wind-down (62°F)
```

### 🌍 Automatic Timezone Support

- **Geographic Detection**: System automatically detects your timezone
- **Fallback Options**: Multiple backup methods ensure reliable operation
- **Global Support**: Works worldwide regardless of device location

## 🔋 Battery Life & Power Management

### Power Saving Features

- **Adaptive Modes**: Automatic CPU frequency scaling (80MHz → 40MHz)
- **Smart Sleep**: Temperature sensor sleeps between readings
- **Display Management**: Screen dims and turns off during idle periods
- **Periodic Monitoring**: Switches between 5-second and 2-minute polling

### Battery Life Expectations

- **Active Use**: 24-48 hours with regular interaction
- **Power Save Mode**: Up to 72 hours with minimal interaction
- **USB Power**: Continuous operation when connected to power

## 📡 LoRaWAN Communication

### Wireless Features

- **Long Range**: LoRaWAN communication for remote installations
- **Low Power**: Efficient communication protocol
- **Reliable**: Multiple transmission attempts with acknowledgments
- **Secure**: Encrypted communication between devices

### Communication Protocol

- **Bidirectional**: Two-way communication between thermostat and receiver
- **Status Updates**: Real-time heating system status
- **Command Transmission**: Remote heating on/off control
- **Error Handling**: Automatic retry and error recovery

## 🔒 Privacy & Security

### Local Operation

- **No Cloud Required**: Operates entirely on your local network
- **Private Data**: Temperature data stays on your devices
- **Secure Credentials**: WiFi passwords stored in protected files

### Data Storage

- **Local Storage**: All data stored locally on device
- **No External Services**: No data sent to external servers
- **User Control**: Complete control over your data and privacy

## 💡 Usage Tips

### Optimal Performance

- **Signal Strength**: Ensure good LoRaWAN signal between devices
- **Sensor Placement**: Mount away from heat sources for accuracy
- **Regular Updates**: Keep firmware updated for latest features

### Maintenance

- **Schedule Review**: Periodically review and adjust temperature schedule
- **Battery Monitoring**: Check battery levels and charging status
- **System Health**: Monitor for communication errors or sensor issues

## 📚 Learn More

### Documentation

For technical details, development setup, and advanced configuration, see:

- **[Developer Documentation](DEVELOPER.md)** - Complete development setup guide
- **[Power Management Guide](doc/POWER_MANAGEMENT.md)** - Battery optimization
  details
- **[Build Instructions](README_BUILD.md)** - Compilation and upload procedures
- **[Timezone Configuration](README_TIMEZONE.md)** - Geographic timezone details
- **[NTP Troubleshooting](README_NTP_TROUBLESHOOTING.md)** - Network time issues

### Support

- **GitHub Issues**: Report bugs and request features
- **Open Source**: Full source code available for customization
- **Community**: Comprehensive guides for setup and troubleshooting

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file
for details.

## 👨‍💻 Author

John Cornelison (<john@vashonSoftware.com>)  
Version 2.0.0 - December 2024 🎉

---

_Built with ❤️ for efficient home heating control_
