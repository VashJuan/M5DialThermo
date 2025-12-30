# LoRa Transmitter Integration for M5Dial Thermostat

This implementation adds LoRaWAN transmitter functionality to the M5Dial
thermostat system, enabling remote control of stove/heater relays through LoRa
communication.

## Overview

The LoRa transmitter system consists of two main components:

1. **LoRaTransmitter Class** - Handles LoRaWAN communication using Grove-Wio-E5
   module
2. **Enhanced Stove Class** - Integrates with LoRaTransmitter for remote control
   and status display

**Note**: The RelayControl class is only used on the receiver side. The M5Dial
transmitter sends commands via LoRa and displays status responses.

## Hardware Requirements

### M5Dial Transmitter Setup

- **M5Dial** - Main controller unit
- **Grove-Wio-E5** - LoRaWAN module for transmission
- **MCP9808** - Temperature sensor

### Connection Diagram

```
M5Dial (Transmitter)
├── Pin 43 (TX) → Grove-Wio-E5 RX
├── Pin 44 (RX) ← Grove-Wio-E5 TX

- **ESP32/Arduino** - Receiver controller
- **Grove-Wio-E5** - LoRaWAN module for reception
- **Relay Module** - For actual stove/heater control

## Software Architecture

### Class Hierarchy

```

Transmitter Side (M5Dial): LoRaTransmitter ├── Uses: HardwareSerial,
LoRaWANConfig ├── Implements: AT command protocol from Grove-Wio-E5 examples └──
Features: Timing measurements, statistics, retry logic

Stove (Enhanced) ├── Uses: LoRaTransmitter for remote commands ├── Displays:
Status responses from remote receiver └── Maintains: Temperature scheduling and
safety logic

Receiver Side (Separate Device): RelayControl ├── Manages: GPIO relay control,
timing, safety ├── Integrates: With LoRaReceiver for remote commands └──
Controls: Physical stove/heater relay

````

## Configuration

### LoRaWAN Network Setup

1. **Update Network Credentials** in your code:

```cpp
LoRaWANConfig config = {
    .appEUI = "YOUR_APP_EUI_HERE",                    // 16 hex chars
    .appKey = "YOUR_APP_KEY_HERE",                    // 32 hex chars
    .region = LORAWAN_REGION_US915,                   // or EU868
    .dataRate = LORAWAN_DR_MEDIUM,
    .adaptiveDataRate = true,
    .transmitPower = 14,
    .otaa = true,
    .confirmUplinks = 1,
    .maxRetries = 3
};
````

2. **Gateway Configuration** - Follow
   [Grove-Wio-E5 documentation](https://wiki.seeedstudio.com/Grove_LoRa_E5_New_Version/)
   for gateway setup.

### Pin Configuration

Default pins can be changed in the code:

```cpp
// LoRa Module Pins (M5Dial Transmitter)
const int LORA_RX_PIN = 44;  // M5Dial → Grove-Wio-E5 TX
const int LORA_TX_PIN = 43;  // M5Dial ← Grove-Wio-E5 RX

// Note: Relay control pin only needed on receiver side
```

## Usage Examples

### Basic Integration

```cpp
#include "lora_transmitter.hpp"
#include "stove.hpp"

LoRaTransmitter transmitter;
Stove stove(&transmitter); // Pass transmitter to stove

void setup() {
    // Initialize LoRa transmitter
    if (transmitter.setup(LORA_RX_PIN, LORA_TX_PIN, config)) {
        stove.setLoRaControlEnabled(true);
        Serial.println("LoRa remote control enabled");
    }
}

void loop() {
    // Stove class handles automatic temperature control via LoRa
    float currentTemp = tempSensor.readTemperatureFahrenheit();
    int hourOfWeek = rtc.getDayOfWeek() * 24 + rtc.getHour();

    String statusText = stove.update(currentTemp, hourOfWeek);
    display.showText(STOVE, statusText); // Display LoRa status
}
```

### Remote Control Commands

The system supports these commands:

- `STOVE_ON` - Turn stove/heater on
- `STOVE_OFF` - Turn stove/heater off
- `STATUS_REQUEST` - Get current stove status
- `PING` - Test connectivity

### Safety Features

- **Temperature Limits** - Prevents turning on if temperature exceeds safety
  threshold
- **Timing Controls** - Enforces minimum intervals between state changes
- **Validation** - Checks command validity before transmission
- **Error Handling** - Comprehensive error reporting and recovery

## Implementation Features

### Inspired by Grove-Wio-E5 Examples

The implementation incorporates several ideas from the
[andresoliva/Grove-Wio-E5](https://github.com/andresoliva/Grove-Wio-E5)
repository:

1. **Enhanced AT Command Handling**

   - Improved serial buffer clearing
   - Timing measurements for commands
   - Better response parsing

2. **Transmission Timing**

   - Measures command execution time
   - Tracks ACK reception timing
   - Provides transmission statistics

3. **Retry Logic**

   - Multiple join attempts with delays
   - Command retry with backoff
   - Robust error handling

4. **Power Management**
   - Low power mode support
   - Auto sleep/wake functionality
   - Configurable power saving

### Code Structure Improvements

1. **Modular Design**

   - Separated relay control from stove logic
   - Independent LoRa transmitter class
   - Shared protocol definitions

2. **Enhanced Error Handling**

   - Detailed error messages
   - Graceful degradation
   - Recovery mechanisms

3. **Statistics and Monitoring**
   - Transmission success/failure rates
   - Signal quality monitoring
   - Performance metrics

## File Structure

````
src/
├── lora_transmitter.hpp/cpp  # LoRaWAN transmitter implementation
├── stove.hpp/cpp             # Enhanced stove class (LoRa integration)
└── lora_thermostat_transmitter.cpp # Full integration example

shared/
└── protocol_common.hpp       # Shared communication protocol

examples/
└── simple_lora_transmitter_example.cpp # Basic usage example

receiver/
├── src/
│   ├── lora_receiver.hpp/cpp    # LoRaWAN receiver implementation
│   ├── relay_control.hpp/cpp    # Physical relay control (receiver only)
│   └── receiver_main.cpp        # Receiver integration

### PlatformIO (Recommended)

```ini
[env:m5dial]
platform = espressif32
board = m5stack-dial
framework = arduino

lib_deps =
    m5stack/M5Unified
    m5stack/M5GFX
    adafruit/Adafruit MCP9808 Library

build_flags =
    -DARDUINO_M5DIAL
    -DCORE_DEBUG_LEVEL=3

monitor_speed = 9600
````

### Arduino IDE

1. Install M5Stack board support
2. Select "M5Dial" board
3. Install required libraries:
   - M5Unified
   - M5GFX
   - Adafruit MCP9808
4. Copy source files to sketch folder
5. Compile and upload

## Troubleshooting

### Common Issues

1. **LoRa Module Not Responding**

   - Check wiring connections
   - Verify power supply (3.3V/5V)
   - Test with AT commands manually

2. **Network Join Failures**

   - Verify AppEUI and AppKey
   - Check regional settings (US915 vs EU868)
   - Ensure gateway is in range

3. **Command Timeouts**
   - Check signal strength
   - Increase retry count
   - Verify receiver is running

### Debug Options

Enable detailed logging:

```cpp
#define LORA_DEBUG_ENABLED
#define STOVE_DEBUG_ENABLED
```

### Performance Monitoring

Check transmission statistics:

```cpp
Serial.println(transmitter.getStatistics());
Serial.println(transmitter.getSignalQuality());
```

## Future Enhancements

- Encryption for secure commands
- Bidirectional temperature monitoring
- Multiple device support
- Web interface for remote control
- Data logging and analytics

## References

- [Grove-Wio-E5 Documentation](https://wiki.seeedstudio.com/Grove_LoRa_E5_New_Version/)
- [Grove-Wio-E5 Examples](https://github.com/andresoliva/Grove-Wio-E5)
- [LoRaWAN Specification](https://lora-alliance.org/wp-content/uploads/2020/11/lorawantm_specification_-v1.0.3.pdf)
- [M5Dial Documentation](https://docs.m5stack.com/en/core/M5Dial)
