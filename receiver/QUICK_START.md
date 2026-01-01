# Quick Start Guide - Receiver Setup

This guide will help you quickly set up the thermostat receiver device.

## Hardware Assembly

### Required Components:

1. **Seeed XIAO ESP32S3** development board
2. **Grove-Wio-E5** LoRaWAN module
3. **Jumper wires** or **Grove cable**
4. **LED** (optional, for status indication)
5. **Breadboard** or **PCB** for connections

### Wiring Diagram:

```
XIAO ESP32S3    Grove-Wio-E5
============    ============
3.3V     ←→     VCC
GND      ←→     GND
D6 (GPIO44/RX)  ←→     TX
D7 (GPIO43/TX)  ←→     RX

XIAO ESP32S3    Gas Stove Interface
============    ==================
D10      ←→     Control Signal Input (HIGH = ON, LOW = OFF)
GND      ←→     Signal Ground

XIAO ESP32S3    Status LED (Optional)
============    ====================
D9       ←→     LED Anode (+ side)
GND      ←→     LED Cathode (- side) through 220Ω resistor
```

## Software Setup

### 1. Install PlatformIO

- Install VS Code
- Install PlatformIO extension

### 2. Open Project

```bash
cd thermo/receiver
code .  # Opens in VS Code with PlatformIO
```

### 3. Configure LoRaWAN

Edit `src/lora_receiver.cpp` lines 52-65:

```cpp
// Replace with your actual network keys
if (!sendATCommand("AT+APPEUI=YOUR_APP_EUI_HERE", "OK")) {
    return false;
}

if (!sendATCommand("AT+APPKEY=YOUR_APP_KEY_HERE", "OK")) {
    return false;
}
```

### 4. Build and Upload

```bash
pio run --target upload
or
pio run --target upload --upload-port COM6
```

## Testing

### 1. Serial Monitor

```bash
cd receiver
pio device monitor
```

Expected output:

```
====================================
Thermostat Receiver Starting...
Hardware: XIAO ESP32S3 + Grove-Wio-E5
====================================
Status LED initialized on pin 9
Stove relay initialized on pin 10 (initial state: OFF)
Grove-Wio-E5 communication established
Successfully joined LoRaWAN network
Auto low power mode enabled - module will sleep automatically
Initial signal quality: RSSI: -65, SNR: 8.5, DR: EU868 DR4 SF8 BW125K
====================================
System Ready - Waiting for commands
Safety timeout: 10 minutes
====================================
```

### 2. Enhanced Features

**Power Management:**

- Automatic low power mode (21µA sleep current)
- Signal quality monitoring every 5 minutes
- Enhanced command timing and reliability

### 3. Test Commands

The receiver will respond to these LoRaWAN commands:

- `STOVE_ON` → Pin D10 goes HIGH
- `STOVE_OFF` → Pin D10 goes LOW
- `STATUS_REQUEST` → Returns current state

### 3. Pin Testing

Use a multimeter to verify pin D10:

- Should be LOW (0V) on startup
- Should go HIGH (3.3V) when stove turned ON
- Should return to LOW when stove turned OFF

## Status LED Guide

| LED Pattern                 | Meaning                                |
| --------------------------- | -------------------------------------- |
| Slow pulse                  | System initializing                    |
| Brief flash every 2 seconds | Waiting for commands                   |
| Fast blink                  | Receiving data                         |
| Solid ON                    | Stove is ON                            |
| Solid OFF                   | Stove is OFF                           |
| Fast flash                  | Safety timeout (stove auto-turned OFF) |
| SOS pattern                 | System error                           |

## Troubleshooting

### Common Issues:

**1. LoRaWAN Join Failed**

```
Solution: Check APPEUI and APPKEY configuration
Verify region setting (US915 vs EU868)
```

**2. Pin D10 Not Working**

```
Check connections with multimeter
Verify XIAO is receiving 5V power
Test pin with simple digitalWrite() in loop
```

**3. Grove-Wio-E5 Not Responding**

```
ERROR: Failed to communicate with Grove-Wio-E5 module
```

**Solutions:**

1. **Verify Pin Connections** - D6/D7 must use GPIO43/GPIO44
   - Physical pin D7 = GPIO43 (TX)
   - Physical pin D6 = GPIO44 (RX)
2. **Check RX/TX Not Swapped** - Grove TX → ESP32 RX, Grove RX → ESP32 TX
3. **Verify Power** - Grove-Wio-E5 needs stable 3.3V (check with multimeter)
4. **Increase Boot Delay** - Module needs 5+ seconds to initialize
5. **Test Manually**:
   ```cpp
   Serial1.begin(9600, SERIAL_8N1, 44, 43);
   delay(5000);
   Serial1.println("AT");
   delay(500);
   while(Serial1.available()) {
       Serial.print((char)Serial1.read());
   }
   ```
6. **Check Baud Rate** - Grove-Wio-E5 defaults to 9600, try 115200 if 9600 fails

**4. System Keeps Resetting**

```
Check power supply stability (need good 5V supply)
Monitor for watchdog timeouts in serial output
```

## Safety Notes

⚠️ **Important Safety Features:**

- System **always starts with stove OFF** (D10 = LOW)
- **10-minute safety timeout** - stove automatically turns OFF if no commands
  received
- **Watchdog protection** - system resets if software hangs
- **State verification** - confirms pin changes actually occurred

## Next Steps

1. **Test with main thermostat**: Set up the M5Stack Dial transmitter
2. **Integrate with gas stove**: Connect D10 to your stove's control input
3. **Optimize range**: Adjust antenna positioning for best LoRaWAN reception
4. **Monitor operation**: Watch serial output during normal operation

## Getting Help

If you encounter issues:

1. Check serial monitor output for error messages
2. Verify all pin connections with multimeter
3. Test LoRaWAN range with ping commands
4. Refer to full documentation in `README.md`
