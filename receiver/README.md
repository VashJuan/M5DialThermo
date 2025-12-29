# Thermostat Receiver

This folder contains the receiving device/relay component of the M5Stack Dial
thermostat system.

## Hardware

- **XIAO ESP32S3** - Main microcontroller

  - Product:
    [Seeed XIAO ESP32S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html)
  - Compact ESP32-S3 based development board
  - Built-in WiFi/Bluetooth (not used in this application)
  - USB-C programming/power

- **Grove-Wio-E5 Wireless Module** - LoRaWAN communication
  - Product:
    [Grove-Wio-E5 STM32WLE5JC](https://www.seeedstudio.com/Grove-LoRa-E5-STM32WLE5JC-p-4867.html)
  - LoRaWAN module with STM32WLE5JC processor
  - EU868 & US915 frequency support
  - AT command interface via UART

## Pin Connections

### XIAO ESP32S3 Pin Assignments:

- **D10**: Gas stove control output (HIGH = ON, LOW = OFF)
- **D9**: Status LED (optional, for visual feedback)
- **D6**: UART RX (connects to Grove-Wio-E5 TX)
- **D7**: UART TX (connects to Grove-Wio-E5 RX)
- **3V3**: Power for Grove-Wio-E5
- **GND**: Ground connection

### Grove-Wio-E5 Connections:

- **VCC**: Connect to XIAO 3.3V
- **GND**: Connect to XIAO GND
- **TX**: Connect to XIAO D6 (RX)
- **RX**: Connect to XIAO D7 (TX)

## Functionality

### Primary Functions:

1. **LoRaWAN Communication**: Receives commands from M5Stack Dial thermostat
2. **Gas Stove Control**: Controls pin D10 to turn stove ON/OFF
3. **Safety Features**: Automatic timeout if no commands received
4. **Status Feedback**: LED indicates system status
5. **Command Acknowledgment**: Sends responses back to transmitter

### Supported Commands:

- `STOVE_ON`: Turn gas stove ON (D10 = HIGH)
- `STOVE_OFF`: Turn gas stove OFF (D10 = LOW)
- `STATUS_REQUEST`: Request current stove status

### Safety Features:

- **Automatic Timeout**: Turns stove OFF if no commands received for 10 minutes
- **Startup Safety**: Always starts with stove OFF
- **Watchdog Timer**: Resets system if software hangs
- **State Verification**: Confirms pin state changes
- **Minimum Change Interval**: Prevents rapid ON/OFF cycling

## Building and Uploading

### Prerequisites:

- PlatformIO IDE or PlatformIO Core
- XIAO ESP32S3 board definitions
- Grove-Wio-E5 configured for LoRaWAN

### Build Commands:

```bash
# Build the project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Configuration:

1. **LoRaWAN Keys**: Update `lora_receiver.cpp` with your network keys:

   - APPEUI (Application EUI)
   - APPKEY (Application Key)
   - Region settings (US915/EU868)

2. **Safety Timeout**: Adjust `SAFETY_TIMEOUT` in `receiver_main.cpp` if needed

## Status LED Patterns

| Pattern                           | Meaning                 |
| --------------------------------- | ----------------------- |
| Slow pulse                        | Initializing system     |
| Slow blink (brief flash every 2s) | Waiting for commands    |
| Fast blink                        | Receiving data          |
| Solid ON                          | Stove is ON             |
| Solid OFF                         | Stove is OFF            |
| Fast flash                        | Safety timeout occurred |
| SOS pattern (... --- ...)         | System error            |

## Serial Monitor Output

The system provides detailed serial output for debugging:

```
====================================
Thermostat Receiver Starting...
Hardware: XIAO ESP32S3 + Grove-Wio-E5
====================================
Setting up status LED on pin 9
Status LED initialized on pin 9
Initializing stove relay...
Setting up stove relay on pin 10
Stove relay initialized on pin 10 (initial state: OFF)
Stove relay initialized - SAFETY: Stove turned OFF
Initializing LoRa receiver...
Setting up LoRa receiver on pins RX:6, TX:7
Grove-Wio-E5 communication established
Successfully joined LoRaWAN network
LoRa receiver setup complete
====================================
System Ready - Waiting for commands
Safety timeout: 10 minutes
====================================

Received command: STOVE_ON
Command executed: Stove turned ON
STOVE TURNED ON - Pin D10 set HIGH
```

## Troubleshooting

### Common Issues:

1. **LoRaWAN Join Failed**:

   - Check APPEUI and APPKEY configuration
   - Verify region settings (US915/EU868)
   - Ensure LoRaWAN gateway is in range
   - Check antenna connection

2. **Pin Control Not Working**:

   - Verify D10 pin connections
   - Check voltage levels with multimeter
   - Ensure proper grounding

3. **No Communication**:

   - Check UART pin connections (D6/D7)
   - Verify Grove-Wio-E5 power supply (3.3V)
   - Check serial baud rate (9600)

4. **System Resets**:
   - Monitor for watchdog timeouts
   - Check power supply stability
   - Review serial output for error messages

### Debug Commands:

Use serial monitor to see detailed system status and LoRaWAN communication.

## Integration with Main Thermostat

This receiver works in conjunction with the main M5Stack Dial thermostat located
in the parent directory. The communication protocol uses LoRaWAN for reliable
long-range communication between the thermostat and gas stove control.

### Communication Flow:

1. M5Stack Dial determines stove should be ON/OFF
2. Transmits LoRaWAN command to receiver
3. Receiver processes command and controls D10 pin
4. Receiver sends acknowledgment back to M5Stack
5. Safety timeout ensures stove turns OFF if communication lost
