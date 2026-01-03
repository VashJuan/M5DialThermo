# 🔧 Hardware Build Guide

## Overview

This guide covers all hardware connections, pin assignments, baud rate
configuration, and technical specifications for the M5Dial Smart Thermostat
system.

## System Components

### Transmitter Unit (M5Dial)

- **M5Stack Dial** - ESP32-S3 based controller
- **MCP9808** - Precision temperature sensor (±0.25°C accuracy)
- **Grove-Wio-E5** - LoRa communication module
- **Power:** USB-C or battery

### Receiver Unit

- **Seeed XIAO ESP32S3** - Relay controller
- **Grove-Wio-E5** - LoRa communication module
- **Relay Module** - For heater/stove control
- **Status LED** (optional) - Visual feedback

## Pin Connections

### M5Dial Transmitter Wiring

```
M5Dial ESP32-S3          Grove-Wio-E5
===============          ============
GPIO43 (TX)       →→→    RX (Module input)
GPIO44 (RX)       ←←←    TX (Module output)
3.3V              →→→    VCC
GND               →→→    GND

M5Dial ESP32-S3          MCP9808 Sensor
===============          ==============
GPIO13 (SDA)      ←→→    SDA (I²C Data)
GPIO15 (SCL)      →→→    SCL (I²C Clock)
3.3V              →→→    VCC
GND               →→→    GND
```

**Important Notes:**

- Use UART1 (not UART0) for LoRa module
- TX/RX must be crossed: Device TX → Module RX
- M5Dial has built-in pull-ups for I²C

### XIAO ESP32S3 Receiver Wiring

```
XIAO ESP32S3             Grove-Wio-E5
============             ============
GPIO43 (D7/TX)    →→→    RX
GPIO44 (D6/RX)    ←←←    TX
3.3V              →→→    VCC
GND               →→→    GND

XIAO ESP32S3             Relay Control
============             =============
GPIO10 (D10)      →→→    Relay Signal Input
GND               →→→    Relay Ground

XIAO ESP32S3             Status LED (Optional)
============             ====================
GPIO9 (D9)        →→→    LED Anode (+)
GND               ←←←    LED Cathode (-) via 220Ω resistor
```

**Critical Pin Mappings:**

- XIAO **D6** = **GPIO44** (Hardware UART RX)
- XIAO **D7** = **GPIO43** (Hardware UART TX)
- Must use these specific GPIOs for UART functionality

## UART/Serial Baud Rate Configuration

### Current Configuration (Firmware v2.0)

Both devices are configured for consistent communication:

```cpp
// In receiver/src/lora_receiver.hpp & src/lora_transmitter.hpp

#define LORA_DISABLE_BAUD_SEARCH true  // Fixed baud rate enabled
#define LORA_FIXED_BAUD_RATE 19200     // Current setting: 19200 baud
#define LORA_INIT_TIMEOUT_MS 180000    // 3-minute timeout
```

**Key Points:**

- **LoRa Module Baud:** 19200 (currently set)
- **USB Serial Monitor:** 115200 (for debugging)
- **Timeout:** 3 minutes (180 seconds)
- **Auto-detection:** Currently DISABLED for predictable synchronization

### Why Fixed Baud Rate?

**Problem with auto-detection:**

- If devices start at different times, they might search different baud rates
- Can miss each other during search cycles
- Unpredictable connection timing

**Solution with fixed baud:**

- Both devices always try same rate
- Can turn on in any order
- Will synchronize whenever both are running
- Predictable 3-minute timeout

### Changing Baud Rate

**To use different baud rate (9600 or 115200):**

1. Edit `receiver/src/lora_receiver.hpp`:

   ```cpp
   #define LORA_FIXED_BAUD_RATE 9600  // or 115200
   ```

2. Edit `src/lora_transmitter.hpp`:

   ```cpp
   #define LORA_TX_FIXED_BAUD_RATE 9600  // or 115200
   ```

3. **Configure Grove-Wio-E5 modules** (both):

   ```
   Connect via USB-TTL adapter
   Send: AT+UART=9600    (or 115200)
   Module responds: +OK
   ```

4. Rebuild and upload both firmwares

**Common Grove-Wio-E5 Baud Rates:**

- 9600 - Lower speed, more reliable
- 19200 - Medium speed (current default)
- 115200 - Highest speed, may be less reliable

### Testing Baud Rate

**Serial Monitor Output (Successful):**

```
Using fixed baud rate: 19200 (baud search disabled)
Connection attempt 1 (elapsed: 2001 ms)...
SUCCESS! Module responding at 19200 baud
```

**Serial Monitor Output (Failed):**

```
Using fixed baud rate: 19200 (baud search disabled)
Connection attempt 1 (elapsed: 2001 ms)...
Command failed - expected 'OK' but got 'AT'
  Received data: 0x41 0x54 0x0D 0x0A
  (May indicate wrong baud rate)
```

If you see "Command failed" messages, the Grove-Wio-E5 is configured for a
different baud rate than your firmware.

### Grove-Wio-E5 Power Consumption

The system uses the
[andresoliva/LoRa-E5 library](https://github.com/andresoliva/LoRa-E5) which
provides automatic power management for battery-powered deployments.

**Power Modes:**

| Mode           | Current Draw | Notes                             |
| -------------- | ------------ | --------------------------------- |
| Active TX      | ~40mA        | During LoRa transmission          |
| Active RX      | ~15mA        | Listening for messages            |
| **Auto Sleep** | **21µA**     | Between operations (default mode) |
| Deep Sleep     | ~2µA         | Manual control (not used)         |

**Battery Life Estimation (with auto sleep):**

Assuming transmit every 5 minutes:

- TX time: ~1s @ 40mA
- Sleep time: 299s @ 0.021mA
- Average: (1×40 + 299×0.021) / 300 = **0.154 mA**

**On common batteries:**

- 2000mAh LiPo: ~540 days
- 1000mAh LiPo: ~270 days
- 500mAh LiPo: ~135 days

> **Note:** The M5Dial transmitter is typically powered by USB-C, but the XIAO
> receiver can run battery-powered for long deployments with the ultra-low sleep
> mode.

## Grove-Wio-E5 Module Configuration

### Initial Setup

**Default Configuration:**

- Frequency: 915 MHz (US) / 868 MHz (EU)
- Baud Rate: Varies (9600, 19200, or 115200)
- Mode: AT Command interface

### Setting Baud Rate

**Using USB-TTL Adapter:**

1. Connect adapter to Grove-Wio-E5:

   ```
   Adapter TX → Grove RX
   Adapter RX → Grove TX
   3.3V → VCC
   GND → GND
   ```

2. Open serial terminal (115200 first, try others if no response)

3. Send AT commands:
   ```
   AT                    (test communication)
   AT+UART=19200        (set baud rate)
   ```

### P2P Mode Configuration

**RF Parameters** (in `shared/protocol_common.hpp`):

```cpp
const uint32_t P2P_FREQUENCY = 915000000;     // 915 MHz (US)
const uint8_t P2P_SPREADING_FACTOR = 12;      // SF12 (max range)
const uint8_t P2P_BANDWIDTH = 125;            // 125 kHz
const uint8_t P2P_CODING_RATE = 1;            // 4/5 coding rate
const uint8_t P2P_POWER = 14;                 // 14 dBm TX power
```

**Regional Settings:**

- **US (915 MHz):** Default - `P2P_FREQUENCY = 915000000`
- **EU (868 MHz):** Change to `P2P_FREQUENCY = 868300000`
- **AS (923 MHz):** Change to `P2P_FREQUENCY = 923000000`

## Hardware Assembly

### Transmitter Assembly (M5Dial)

**Step 1: Connect Temperature Sensor**

1. Solder wires or use Grove cable
2. Connect to I²C pins (SDA=GPIO13, SCL=GPIO15)
3. Apply heatshrink or electrical tape

**Step 2: Connect LoRa Module**

1. Connect to UART pins (TX=GPIO43, RX=GPIO44)
2. Ensure TX/RX are crossed properly
3. Connect power (3.3V) and ground

**Step 3: Power Connection**

1. USB-C for primary power, or
2. Battery connection if supported

**Step 4: Antenna**

1. Attach LoRa antenna to Grove-Wio-E5
2. Ensure good connection

### Receiver Assembly (XIAO)

**Step 1: Breadboard Layout**

1. Place XIAO ESP32S3 on breadboard
2. Place Grove-Wio-E5 nearby
3. Add relay module

**Step 2: Power Connections**

1. Connect 3.3V rail to breadboard
2. Connect GND rail
3. Power from USB or external 5V

**Step 3: Signal Connections**

1. XIAO D7 (GPIO43) → Grove-Wio-E5 RX
2. XIAO D6 (GPIO44) → Grove-Wio-E5 TX
3. XIAO D10 → Relay signal input
4. XIAO D9 → LED anode (via resistor)

**Step 4: Verify Connections**

1. Use multimeter to check voltages
2. Verify no shorts between power/ground
3. Check continuity on signal lines

## Power Requirements

### M5Dial Transmitter

**Power Specifications:**

- **Input:** 5V via USB-C
- **Operating:** 3.3V internal regulation
- **Current Draw:**
  - Active: 50-70 mA
  - Power Save: 20-30 mA
  - Deep Sleep: 10-15 mA
- **Battery:** Optional (depends on M5Dial model)

### XIAO Receiver

**Power Specifications:**

- **Input:** 5V via USB-C
- **Operating:** 3.3V internal regulation
- **Current Draw:**
  - Active (listening): 40-60 mA
  - Receiving: 70-90 mA
  - Deep Sleep: 20-30 mA (with Grove-Wio-E5 awake)
- **Power Supply:** Must be stable, use quality USB power adapter

### Grove-Wio-E5 Module

**Power Specifications:**

- **Operating Voltage:** 3.3V
- **Current Draw:**
  - Sleep: 21 µA (with auto low power mode)
  - RX Mode: 5-15 mA
  - TX Mode (14dBm): 40-45 mA
- **Power-On Time:** 5 seconds minimum

**Important:** Allow 5+ seconds after power-on before sending AT commands.

## Communication Range

### Expected Range

**P2P Mode:**

- **Line of Sight:** 1-2 km outdoors
- **Indoor (residential):** 100-300 meters
- **Indoor (commercial):** 50-150 meters

**LoRaWAN Mode:**

- **Gateway Range:** 2-10 km urban, 15+ km rural
- **Multiple gateways:** Extends effective range

### Range Factors

**Positive Factors:**

- ✓ Higher mounting elevation
- ✓ Clear line of sight
- ✓ Good antenna orientation
- ✓ Higher transmit power (within regulations)
- ✓ Higher spreading factor (SF12)

**Negative Factors:**

- ✗ Thick walls (concrete, brick)
- ✗ Metal structures or shielding
- ✗ Multiple floors between devices
- ✗ Interference from 900 MHz devices
- ✗ Low mounting (ground level)

## Antenna Considerations

### Antenna Types

**Included Antennas:**

- Grove-Wio-E5 typically includes helical antenna
- Frequency: 915 MHz or 868 MHz
- Gain: ~2 dBi

**Upgrade Options:**

- External antenna with RP-SMA connector
- Higher gain (3-5 dBi) for better range
- Directional antennas for specific installations

### Antenna Placement

**Best Practices:**

- Mount vertically (perpendicular to ground)
- Keep away from metal objects
- Avoid coiling or bending antenna
- Use antenna extension cable if needed

## Safety Considerations

### Electrical Safety

⚠️ **Important Warnings:**

- Never exceed 3.3V on GPIO pins
- Use appropriate wire gauge for relay loads
- Isolate high-voltage heater circuits from low-voltage electronics
- Use optoisolator for heater control relay

### Relay Safety

**Relay Specifications:**

- Must handle heater current rating
- Include flyback diode for inductive loads
- Use appropriate safety margin (2x rated current)

**Recommended:**

- Solid-state relay (SSR) for heater control
- Mechanical relay with 10A+ rating minimum
- Properly rated for AC voltage (if applicable)

### Fire Safety

⚠️ **Critical:**

- Ensure heater has built-in safety features
- Install smoke detectors in area
- Never leave unattended for extended periods
- Implement proper timeout safety (10 minutes)

## Testing Procedures

### Initial Power-On Test

**Step 1: Visual Inspection**

1. ✓ Check all connections
2. ✓ Verify no shorts
3. ✓ Confirm polarity correct

**Step 2: Power Test**

1. Connect USB power (without LoRa modules first)
2. Check voltage at 3.3V points with multimeter
3. Verify current draw is reasonable

**Step 3: LoRa Module Test**

1. Connect one module, power on
2. Check serial output at 19200 baud
3. Send AT command, verify response
4. Repeat for second module

### Communication Range Test

**Test Procedure:**

1. Power both units
2. Place 1 meter apart initially
3. Verify communication works
4. Gradually increase distance
5. Note maximum reliable range
6. Mark any dead zones

### Relay Control Test

**Test Procedure:**

1. Connect multimeter to relay output
2. Send STOVE_ON command from transmitter
3. Verify relay switches (voltage change)
4. Send STOVE_OFF command
5. Verify relay releases

**DO NOT connect actual heater until fully tested!**

## Troubleshooting Hardware Issues

### No Communication

**Check:**

1. TX/RX connections (must be crossed)
2. Baud rate matches on both devices (19200)
3. Grove-Wio-E5 powered with stable 3.3V
4. Antennas attached
5. Allow 5+ seconds after power-on

**Test:**

```cpp
// Simple test code
Serial1.begin(19200, SERIAL_8N1, 44, 43);
delay(5000);  // Wait for module
Serial1.println("AT");
delay(500);
while (Serial1.available()) {
    Serial.print((char)Serial1.read());
}
// Should see "+OK" or similar response
```

### Relay Not Switching

**Check:**

1. Voltage at GPIO10 when ON (should be 3.3V)
2. Ground connection to relay
3. Relay power supply
4. Relay coil voltage rating

**Test with multimeter:**

- Measure GPIO10 voltage
- Should be 3.3V when high, 0V when low

### Intermittent Connection

**Possible Causes:**

1. Poor antenna connection
2. Interference from other devices
3. Power supply noise or instability
4. Marginal signal strength
5. Timing issues with LoRa module

**Solutions:**

1. Add ferrite beads on power lines
2. Use shielded cables if possible
3. Add bypass capacitors (100nF) near modules
4. Increase spreading factor for more reliability

## Enclosure Considerations

### M5Dial Housing

- M5Stack provides case options
- Ensure temperature sensor exposure
- Allow air circulation around sensor
- Provide access to USB port

### Receiver Housing

- Protect from moisture/dust
- Allow LED visibility
- Provide antenna clearance
- Ventilation for relay heat dissipation

## Maintenance

### Regular Checks

**Weekly:**

- ✓ Verify status LED patterns correct
- ✓ Check wireless communication working

**Monthly:**

- ✓ Clean dust from sensors and display
- ✓ Check all connections secure
- ✓ Verify relay operation smooth

**Annually:**

- ✓ Check for corrosion on connections
- ✓ Test full range of operation
- ✓ Update firmware if available

---

**Version:** 2.0.0  
**Last Updated:** January 2026  
**Author:** John Cornelison (john@vashonSoftware.com)
