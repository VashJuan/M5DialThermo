# 👨‍💻 Developer Guide

## Overview

This guide covers building, uploading, and developing the M5Dial Smart
Thermostat firmware. Includes development environment setup, build procedures,
and code architecture.

## Development Environment

### Required Tools

**1. Visual Studio Code**

- Download: https://code.visualstudio.com/
- Primary IDE for development

**2. PlatformIO Extension**

- Install from VS Code Extensions marketplace
- ID: `platformio.platformio-ide`
- Required for building and uploading

**3. C/C++ Extension (Recommended)**

- Install from VS Code Extensions marketplace
- ID: `ms-vscode.cpptools`
- Provides IntelliSense and debugging

### Project Setup

**1. Open Workspace:**

```bash
cd "d:\Projects\Arduino\M5 Stack\thermo"
code __HouseThermo.code-workspace
```

**2. Workspace Structure:**

```
thermo/
├── __HouseThermo.code-workspace  # VS Code workspace config
├── platformio.ini                 # PlatformIO configuration
├── src/                          # Transmitter (M5Dial) source
│   ├── _thermo.cpp              # Main application
│   ├── display.cpp/.hpp         # Display management
│   ├── encoder.cpp/.hpp         # Dial encoder
│   ├── stove.cpp/.hpp           # Heating control logic
│   ├── temp_sensor.cpp/.hpp     # Temperature sensor
│   ├── rtc.cpp/.hpp             # Real-time clock
│   ├── lora_transmitter.cpp/.hpp # LoRa transmitter
│   ├── secrets_template.h       # Template for credentials
│   └── secrets.h                # Your credentials (not in git)
├── receiver/                     # Receiver (XIAO) project
│   ├── platformio.ini           # Receiver config
│   └── src/                     # Receiver source
│       ├── receiver_main.cpp    # Main application
│       ├── lora_receiver.cpp/.hpp
│       ├── stove_relay.cpp/.hpp
│       └── status_led.cpp/.hpp
├── shared/                       # Shared code
│   └── protocol_common.hpp      # Communication protocol
└── data/                        # Filesystem data
    └── temps.csv                # Temperature schedule
```

## Building the Projects

### Transmitter (M5Dial)

**Build Commands:**

```bash
# Build firmware
pio run

# Upload firmware to COM4
pio run --target upload --upload-port COM4

# Upload filesystem (temps.csv)
pio run --target uploadfs --upload-port COM4

# Build + Upload + Monitor
pio run --target upload --upload-port COM4 && pio device monitor --port COM4 --baud 115200
```

**Configuration:** `platformio.ini`

```ini
[env:m5stack-stamps3]
platform = espressif32
board = m5stack-stamps3
framework = arduino
monitor_speed = 115200

lib_deps =
    m5stack/M5Unified @ ^0.1.17
    m5stack/M5GFX @ ^0.1.17
    adafruit/Adafruit MCP9808 Library @ ^2.0.2
```

### Receiver (XIAO ESP32S3)

**Build Commands:**

```bash
cd receiver

# Build firmware
pio run

# Upload firmware to COM6
pio run --target upload --upload-port COM6

# Monitor
pio device monitor --port COM6 --baud 115200
```

**Configuration:** `receiver/platformio.ini`

```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200
```

## Configuration Files

### secrets.h Setup

**Never commit `secrets.h` to git!**

**1. Create from template:**

```bash
# Transmitter
cp src/secrets_template.h src/secrets.h

# Receiver
cp receiver/src/secrets_template.h receiver/src/secrets.h
```

**2. Edit with your credentials:**

```cpp
// src/secrets.h (Transmitter)
#define DEFAULT_WIFI_SSID "Your_WiFi_Name"
#define DEFAULT_WIFI_PASSWORD "Your_WiFi_Password"
#define LORAWAN_APP_EUI "0000000000000000"
#define LORAWAN_APP_KEY "00000000000000000000000000000000"
```

**3. Verify in .gitignore:**

```
secrets.h
```

### temps.csv Configuration

**Location:** `data/temps.csv`

**Format:**

```csv
BaseTemperature,70.0

# Hour,Offset,Description
1,-12.0,Sleep
8,0.0,Morning
17,2.0,Evening
22,-8.0,Night
```

**Upload to Device:**

```bash
pio run --target uploadfs --upload-port COM4
```

## Code Architecture

### Transmitter (M5Dial)

**Main Application:** `src/_thermo.cpp`

```cpp
void setup() {
    M5.begin();           // Initialize M5Dial
    Serial.begin(115200); // Debug output
    encoder.setup();      // Rotary encoder
    rtc.setup();          // Real-time clock
    tempSensor.init();    // Temperature sensor
    stove.setup();        // Heating control + LoRa
    display.setup();      // LCD display
}

void loop() {
    encoder.update();     // Check for dial movement
    tempSensor.update();  // Read temperature
    stove.update();       // Control heating
    display.update();     // Update screen
    powerManagement();    // Battery optimization
}
```

**Key Classes:**

**TemperatureSensor** - MCP9808 interface with caching

```cpp
class TemperatureSensor {
    float readTemperatureF();           // Read current temp
    float getLastTemperatureF() const;  // Get cached temp
    void wakeUp();                      // Wake sensor
    void shutdown();                    // Sleep sensor
    bool getAwakeStatus();              // Check if awake
};
```

**Stove** - Heating control with LoRa

```cpp
class Stove {
    void setup(LoRaTransmitter* lora);  // Initialize
    void turnOn();                       // Heat on command
    void turnOff();                      // Heat off command
    bool isOn() const;                   // Get state
    void update();                       // Periodic update
};
```

**LoRaTransmitter** - Wireless communication

```cpp
class LoRaTransmitter {
    bool setup(int rxPin, int txPin);           // Initialize
    String sendCommand(String cmd);             // Send command
    String sendCommandWithFallback(String cmd); // Try both modes
    LoRaCommunicationMode getCurrentMode();     // Get mode (P2P/LoRaWAN)
};
```

**Display** - LCD management

```cpp
class Display {
    void setup();                        // Initialize
    void showTemperature(float temp);    // Show temp
    void showTarget(float target);       // Show setpoint
    void showStatus(String status);      // Show heating status
    void update();                       // Refresh display
};
```

### Receiver (XIAO ESP32S3)

**Main Application:** `receiver/src/receiver_main.cpp`

```cpp
void setup() {
    statusLED.setup();     // Status indicator
    stoveRelay.setup();    // Relay control
    loraReceiver.setup();  // LoRa receiver
}

void loop() {
    String command = loraReceiver.checkForCommand();

    if (command == "STOVE_ON") {
        stoveRelay.turnOn();
        loraReceiver.sendResponse("ACK");
    }
    else if (command == "STOVE_OFF") {
        stoveRelay.turnOff();
        loraReceiver.sendResponse("ACK");
    }

    safetyTimeout();  // Auto-off after 10 minutes
    statusLED.update();
}
```

**Key Classes:**

**LoRaReceiver** - Wireless receiver

```cpp
class LoRaReceiver {
    bool setup(int rxPin, int txPin);     // Initialize
    String checkForCommand();             // Check for messages
    bool sendResponse(String response);   // Send acknowledgment
};
```

**StoveRelay** - GPIO relay control

```cpp
class StoveRelay {
    bool setup(int pin);    // Initialize relay pin
    void turnOn();          // Set relay HIGH
    void turnOff();         // Set relay LOW
    bool isOn() const;      // Get state
};
```

**StatusLED** - Visual feedback

```cpp
class StatusLED {
    void setup(int pin);               // Initialize LED
    void setStatus(LEDStatus status);  // Set pattern
    void update();                     // Update animation
};
```

## Communication Protocol

### Protocol Definition

**Location:** `shared/protocol_common.hpp`

**Commands:**

```cpp
// Control commands
#define CMD_STOVE_ON "STOVE_ON"
#define CMD_STOVE_OFF "STOVE_OFF"
#define CMD_STATUS_REQUEST "STATUS_REQUEST"
#define CMD_PING "PING"

// Responses
#define RESP_ACK "ACK"
#define RESP_STOVE_ON "STOVE_ON"
#define RESP_STOVE_OFF "STOVE_OFF"
#define RESP_PONG "PONG"
```

**P2P Configuration:**

```cpp
const uint32_t P2P_FREQUENCY = 915000000;     // 915 MHz
const uint8_t P2P_SPREADING_FACTOR = 12;      // SF12
const uint8_t P2P_BANDWIDTH = 125;            // 125 kHz
const uint8_t P2P_CODING_RATE = 1;            // 4/5
const uint8_t P2P_POWER = 14;                 // 14 dBm
```

### Message Encoding

**Protocol Helper:**

```cpp
class ProtocolHelper {
    static String asciiToHex(const String& ascii);
    static String hexToAscii(const String& hex);
    static bool isValidCommand(const String& cmd);
    static bool isValidResponse(const String& resp);
};
```

**Usage:**

```cpp
String hex = ProtocolHelper::asciiToHex("STOVE_ON");
// Returns: "53544F56455F4F4E"

String ascii = ProtocolHelper::hexToAscii("53544F56455F4F4E");
// Returns: "STOVE_ON"
```

## Power Management

### Power Modes

**Active Mode** (0-3 seconds after interaction):

```cpp
setCpuFrequencyMhz(80);                    // Normal speed
const unsigned long TEMP_POLL_ACTIVE = 5000;  // 5 sec polling
displayUpdateRate = 2000;                  // 2 sec display updates
```

**Power Save Mode** (3+ seconds idle):

```cpp
setCpuFrequencyMhz(40);                    // Reduced speed
const unsigned long TEMP_POLL_SAVE = 120000;  // 2 min polling
displayUpdateRate = 10000;                 // 10 sec display updates
tempSensor.shutdown();                     // Sleep sensor
```

**Implementation:**

```cpp
unsigned long lastInteraction = 0;
bool powerSaveMode = false;

void powerManagement() {
    if (millis() - lastInteraction > 3000) {
        if (!powerSaveMode) {
            powerSaveMode = true;
            setCpuFrequencyMhz(40);
            tempSensor.shutdown();
        }
    } else {
        if (powerSaveMode) {
            powerSaveMode = false;
            setCpuFrequencyMhz(80);
            tempSensor.wakeUp();
        }
    }
}
```

## Testing & Debugging

### Serial Monitor

**Transmitter Output:**

```
Setting up encoder... and RTC...
Temperature sensor initialized at 0x18
Setting up LoRa transmitter...
SUCCESS! Module responding at 19200 baud
P2P mode configured successfully
System ready
```

**Receiver Output:**

```
Thermostat Receiver Starting...
Status LED initialized on pin 9
Stove relay initialized on pin 10
LoRa receiver initialized
P2P mode configured successfully
System Ready - Waiting for commands
```

### Debug Flags

**Enable verbose output:**

```cpp
#define DEBUG_LORA 1        // LoRa communication
#define DEBUG_TEMP 1        // Temperature readings
#define DEBUG_POWER 1       // Power management
```

### Unit Testing Commands

**Test Temperature Sensor:**

```cpp
Serial.println(tempSensor.readTemperatureF());
Serial.println(tempSensor.getLastReadTime());
```

**Test LoRa Communication:**

```cpp
String resp = lora.sendCommand("PING");
Serial.println(resp);  // Should be "PONG"
```

**Test Relay:**

```cpp
stoveRelay.turnOn();
delay(1000);
Serial.println(stoveRelay.isOn());  // Should be true
stoveRelay.turnOff();
```

## Common Development Tasks

### Adding a New Command

**1. Define in protocol_common.hpp:**

```cpp
#define CMD_NEW_FEATURE "NEW_FEATURE"
#define RESP_NEW_FEATURE "FEATURE_OK"
```

**2. Handle in receiver (receiver_main.cpp):**

```cpp
else if (command == "NEW_FEATURE") {
    // Do something
    loraReceiver.sendResponse("FEATURE_OK");
}
```

**3. Send from transmitter (stove.cpp):**

```cpp
String response = loraTransmitter->sendCommand("NEW_FEATURE");
```

### Modifying Temperature Schedule

**1. Edit data/temps.csv:**

```csv
BaseTemperature,72.0

# Hour,Offset,Description
0,-15.0,Deep sleep
6,-10.0,Wake up
9,0.0,Day
18,3.0,Evening
23,-10.0,Bed
```

**2. Upload to device:**

```bash
pio run --target uploadfs --upload-port COM4
```

**3. Restart device or reload schedule**

### Changing LoRa Parameters

**1. Edit shared/protocol_common.hpp:**

```cpp
const uint32_t P2P_FREQUENCY = 868300000;  // Change to EU
const uint8_t P2P_SPREADING_FACTOR = 10;   // Faster, shorter range
```

**2. Rebuild both projects:**

```bash
# Transmitter
pio run

# Receiver
cd receiver && pio run
```

**3. Upload to both devices**

## Troubleshooting Build Issues

### PlatformIO Not Found

**Solution:**

```bash
# Install PlatformIO Core
pip install platformio

# Or reinstall VS Code extension
```

### Library Dependencies Missing

**Solution:**

```bash
# Install libraries
pio lib install

# Or clean and rebuild
pio run --target clean
pio run
```

### Upload Failed

**Check:**

- ✓ Correct COM port
- ✓ Device connected
- ✓ Driver installed
- ✓ No serial monitor open

**Solutions:**

```bash
# List ports
pio device list

# Specify port explicitly
pio run --target upload --upload-port COM4

# Hold reset button during upload (if needed)
```

### Compilation Errors

**Common Issues:**

```cpp
// Missing secrets.h
cp src/secrets_template.h src/secrets.h

// Mismatched library versions
pio lib update

// Missing shared files
# Ensure shared/ folder exists
```

## Code Style Guide

### Naming Conventions

**Files:**

- Header: `class_name.hpp`
- Implementation: `class_name.cpp`
- Main: `_thermo.cpp`

**Classes:**

```cpp
class TemperatureSensor { };  // PascalCase
```

**Functions:**

```cpp
void readTemperature() { }    // camelCase
```

**Constants:**

```cpp
const int LED_PIN = 9;        // UPPER_SNAKE_CASE
#define MAX_RETRIES 3
```

**Variables:**

```cpp
float currentTemp;            // camelCase
bool powerSaveMode;
```

### Documentation

**Class Documentation:**

```cpp
/**
 * @class TemperatureSensor
 * @brief Manages MCP9808 temperature sensor
 *
 * Provides temperature reading with caching and
 * power management features.
 */
```

**Function Documentation:**

```cpp
/**
 * @brief Read temperature from sensor
 * @return Temperature in Fahrenheit
 */
float readTemperatureF();
```

### Error Handling

**Pattern:**

```cpp
bool functionName() {
    if (!initialize()) {
        Serial.println("Error: Initialization failed");
        return false;
    }

    if (!verify()) {
        Serial.println("Error: Verification failed");
        return false;
    }

    return true;
}
```

## Contributing

### Pull Request Process

1. Fork the repository
2. Create feature branch (`git checkout -b feature/new-feature`)
3. Make changes and test thoroughly
4. Commit with clear messages
5. Push to branch
6. Create pull request

### Testing Checklist

Before submitting:

- ✅ Code compiles without warnings
- ✅ Tested on actual hardware
- ✅ Serial output clean and informative
- ✅ No memory leaks
- ✅ Power consumption acceptable
- ✅ Documentation updated

## Additional Resources

### Documentation

- **User Guide:** Basic operation and features
- **Hardware Guide:** Pin connections and specifications
- **Network Guide:** Communication setup and troubleshooting

### External Links

- **M5Stack Dial:** https://docs.m5stack.com/en/core/M5Dial
- **XIAO ESP32S3:** https://wiki.seeedstudio.com/xiao_esp32s3
- **Grove-Wio-E5:** https://wiki.seeedstudio.com/Grove_Wio_E5
- **MCP9808:** https://www.adafruit.com/product/1782
- **PlatformIO:** https://docs.platformio.org/

---

**Version:** 2.0.0  
**Last Updated:** January 2026  
**Author:** John Cornelison (john@vashonSoftware.com)  
**License:** MIT
