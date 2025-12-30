# P2P LoRa Communication Implementation

## Overview

The M5Dial Thermostat now supports both **Point-to-Point (P2P)** and **LoRaWAN**
communication modes for controlling the remote relay. P2P mode is used as the
default with automatic fallback to LoRaWAN when P2P fails.

## Communication Modes

### P2P Mode (Default)

- **Simpler Setup**: No network infrastructure required
- **Direct Communication**: Point-to-point between transmitter and receiver
- **Lower Latency**: Faster response times
- **Better for Local Control**: Ideal for single home/building applications

### LoRaWAN Mode (Fallback)

- **Network Infrastructure**: Requires LoRaWAN gateway and network server
- **Scalable**: Can support many devices and complex networks
- **Internet Integration**: Can integrate with cloud services
- **Better for Multi-Device**: Ideal for distributed sensor networks

## P2P Configuration

The P2P mode uses these default parameters (configurable in
`shared/protocol_common.hpp`):

```cpp
// P2P Configuration
const uint32_t P2P_FREQUENCY = 868300000;      // 868.3 MHz (EU) / Change to 915000000 for US
const uint8_t P2P_SPREADING_FACTOR = 12;        // SF12 for maximum range
const uint8_t P2P_BANDWIDTH = 125;              // 125 kHz
const uint8_t P2P_CODING_RATE = 1;              // 4/5 coding rate
const uint16_t P2P_PREAMBLE_LENGTH = 8;         // 8-symbol preamble
const uint8_t P2P_POWER = 14;                   // 14 dBm transmit power
const uint16_t P2P_TX_TIMEOUT = 3000;           // 3 second transmit timeout
const uint16_t P2P_RX_TIMEOUT = 5000;           // 5 second receive timeout
```

## How It Works

### Automatic Mode Selection

1. **Setup Phase**:

   - System tries to configure P2P mode first
   - If P2P fails, automatically falls back to LoRaWAN
   - Mode is displayed on the M5Dial screen

2. **Command Transmission**:

   - `sendCommandWithFallback()` tries current mode first
   - If current mode fails, automatically tries the other mode
   - Returns response from successful mode or empty string if both fail

3. **Status Display**:
   - Shows current mode (P2P or LoRaWAN) on screen
   - Displays communication status and responses

### P2P Command Flow

```
M5Dial Transmitter                    Receiver
------------------                    ---------
1. Enter TEST mode      →
2. Configure RF params  →
3. Send AT+TEST=TXLRPKT →             Listen for P2P packets
4. Wait for response    ←             Send response via P2P
5. Parse response       ←
```

### LoRaWAN Command Flow (Fallback)

```
M5Dial Transmitter                    LoRaWAN Network                    Receiver
------------------                    ---------------                    ---------
1. Join network         →
2. Send uplink message  →             Route to receiver       →
3. Wait for downlink    ←             Route from receiver     ←         Send response
4. Parse response       ←
```

## Commands Supported

All standard thermostat commands work in both modes:

- `STOVE_ON` - Turn stove/heater on
- `STOVE_OFF` - Turn stove/heater off
- `PING` - Test connectivity
- `STATUS_REQUEST` - Request current status

## Implementation Details

### Key Classes

- **LoRaTransmitter**: Main communication class supporting both P2P and LoRaWAN
- **ProtocolHelper**: Utility functions for message encoding/decoding
- **Stove**: Updated to use fallback communication and display mode status

### Key Methods

- `configureP2P()`: Set up P2P mode using AT+TEST commands
- `sendP2PMessage()`: Send message via AT+TEST=TXLRPKT
- `receiveP2PMessage()`: Receive message via AT+TEST=RXLRPKT
- `switchMode()`: Change between P2P and LoRaWAN modes
- `sendCommandWithFallback()`: Try both modes automatically

## Usage Examples

### Basic Command Sending

```cpp
LoRaTransmitter lora;
lora.setup(rxPin, txPin, config);  // P2P mode configured by default

// This will try P2P first, fallback to LoRaWAN if needed
String response = lora.sendCommandWithFallback("STOVE_ON");
```

### Manual Mode Switching

```cpp
// Switch to LoRaWAN mode manually
if (lora.switchMode(LoRaCommunicationMode::LoRaWAN)) {
    String response = lora.sendCommand("STOVE_ON");
}

// Switch back to P2P mode
if (lora.switchMode(LoRaCommunicationMode::P2P)) {
    String response = lora.sendCommand("STOVE_OFF");
}
```

### Check Current Mode

```cpp
LoRaCommunicationMode mode = lora.getCurrentMode();
if (mode == LoRaCommunicationMode::P2P) {
    Serial.println("Using P2P mode");
} else {
    Serial.println("Using LoRaWAN mode");
}
```

## Benefits of P2P + LoRaWAN Fallback

1. **Reliability**: If one mode fails, the other provides backup
2. **Flexibility**: Choose the best mode for your setup
3. **Future-Proof**: Easy to expand to full LoRaWAN network later
4. **Local Independence**: P2P works without internet/gateway
5. **Range**: Both modes provide excellent long-range communication

## Regional Considerations

### Frequency Configuration

- **US (915 MHz)**: Change `P2P_FREQUENCY` to `915000000`
- **EU (868 MHz)**: Default `868300000` is correct
- **Asia-Pacific**: May need different frequencies based on local regulations

### Power Limits

- Adjust `P2P_POWER` based on local regulations
- Some regions limit power to 10 dBm or lower

## Troubleshooting

### P2P Mode Issues

1. **Range Problems**: Increase spreading factor (SF) or power
2. **Interference**: Try different frequencies within allowed band
3. **Configuration Errors**: Check AT command responses in serial monitor

### LoRaWAN Fallback Issues

1. **Join Failures**: Verify AppEUI and AppKey in secrets.h
2. **Gateway Issues**: Ensure LoRaWAN gateway is accessible
3. **Network Problems**: Check TTN or other network server status

### General Communication Issues

1. **Module Not Responding**: Check wiring and power supply
2. **Both Modes Fail**: Verify Grove-Wio-E5 firmware version
3. **Serial Issues**: Confirm baud rate (9600) and pin configuration

## Development Notes

This implementation was inspired by:

- [Grove Wio-E5 P2P Examples](https://wiki.seeedstudio.com/Grove_Wio_E5_P2P/)
- [Grove Wio-E5 GitHub Repository](https://github.com/andresoliva/Grove-Wio-E5)

The fallback mechanism ensures maximum reliability for critical thermostat
control applications.
