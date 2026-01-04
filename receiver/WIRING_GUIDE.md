# XIAO ESP32S3 Receiver - Wiring Guide

## Hardware Components

- **Seeed XIAO ESP32S3** (main controller)
- **Grove-Wio-E5** (LoRa module)
- **Status LED** (optional, for visual feedback)
- **Relay Module** (for stove control)

---

## Pin Assignments

### LoRa Module (Grove-Wio-E5)

The Grove-Wio-E5 connects to the XIAO ESP32S3 hardware UART pins:

| Grove-Wio-E5 Pin | XIAO ESP32S3 Pin | GPIO   | Description                     |
| ---------------- | ---------------- | ------ | ------------------------------- |
| TX (transmit)    | D6 (RX)          | GPIO44 | LoRa transmits → ESP32 receives |
| RX (receive)     | D7 (TX)          | GPIO43 | ESP32 transmits → LoRa receives |
| VCC              | 3.3V             | -      | Power supply (3.3V)             |
| GND              | GND              | -      | Ground                          |

**Important Notes:**

- Grove-Wio-E5 TX goes to ESP32 RX (GPIO44)
- Grove-Wio-E5 RX goes to ESP32 TX (GPIO43)
- Do NOT use 5V - Grove-Wio-E5 requires 3.3V
- Serial communication is at 9600 baud

---

### Status LED (Optional)

A simple LED with current-limiting resistor for visual status feedback:

| Component       | XIAO ESP32S3 Pin | GPIO  | Description                               |
| --------------- | ---------------- | ----- | ----------------------------------------- |
| LED Anode (+)   | D9               | GPIO9 | LED positive (through 220Ω-330Ω resistor) |
| LED Cathode (-) | GND              | -     | LED negative                              |

**LED Wiring:**

```
XIAO D9 (GPIO9) → [220Ω Resistor] → LED Anode (+) → LED Cathode (-) → GND
```

**LED Status Patterns:**

- **Solid ON**: Stove is ON
- **Blinking Fast**: Receiving command
- **Slow Pulse**: Waiting for commands
- **Rapid Flash**: Error/timeout

**Why LED might be "rarely on":**

- The LED only lights when stove is ON or during command reception
- Most of the time it's in "waiting" mode (slow pulse) or off
- Check wiring if you don't see ANY activity during status changes

---

### Stove Control Relay

**CRITICAL SAFETY NOTE:** The relay controls gas stove ignition. Ensure proper
safety measures!

| Relay Module Pin | XIAO ESP32S3 Pin | GPIO   | Description                                 |
| ---------------- | ---------------- | ------ | ------------------------------------------- |
| Signal/IN        | D10              | GPIO10 | Control signal (HIGH=ON, LOW=OFF)           |
| VCC              | 5V or 3.3V       | -      | Relay power (check your relay module specs) |
| GND              | GND              | -      | Ground                                      |

**Relay Module Types:**

1. **Active High Relay** (most common):

   - HIGH (3.3V) = Relay ON
   - LOW (0V) = Relay OFF
   - This is what the code uses by default

2. **Active Low Relay** (less common):
   - If your relay turns ON when LOW, you'll need to invert the logic in code

**Typical Relay Module Wiring:**

```
XIAO D10 (GPIO10) → Relay Signal Pin (IN)
XIAO 5V or 3.3V   → Relay VCC (check module requirements)
XIAO GND          → Relay GND

Relay COM (Common) → Stove control wire
Relay NO (Normally Open) → Stove control wire (completes circuit when ON)
```

**Why relay might be "rarely on":**

- The stove only turns ON when temperature drops below target
- If your current temp is above target, stove stays OFF
- Check serial monitor for "STOVE_ON" commands to verify relay activation
- Test manually: send "STOVE_ON" command to force activation

---

## Troubleshooting

### LED Not Working

1. **Check polarity**: LED anode (+, longer leg) to D9, cathode (-, shorter leg)
   to GND
2. **Check resistor**: Use 220Ω-330Ω resistor (required!)
3. **Test pin**: Upload code and watch serial monitor for "Status LED test
   flash" messages
4. **Check GPIO9**: Use multimeter to verify 3.3V on D9 during test
5. **Try different LED**: LED might be damaged

### Relay Not Activating

1. **Check power**: Relay module needs VCC (5V or 3.3V depending on module)
2. **Check signal**: Use multimeter on D10 - should show 3.3V when stove should
   be ON
3. **Check relay type**: Active-high vs active-low (listen for relay click)
4. **Test independently**: Write simple test sketch to toggle D10 every second
5. **Check stove logic**: Serial monitor shows actual stove state and commands

### Quick Test Code

Add this to `setup()` after relay initialization:

```cpp
// Test relay
Serial.println("Testing relay...");
stoveRelay.turnOn();
delay(2000);
stoveRelay.turnOff();
delay(2000);
```

### Checking Serial Output

The receiver logs all state changes:

```
P2P RX: STATUS_REQUEST          ← Received command
Received command: STATUS_REQUEST ← Processing command
P2P TX: STOVE_OFF_ACK           ← Sending response
```

Look for:

- `Stove turned ON` or `Stove turned OFF` messages
- `Status LED changed to: STOVE_ON` messages
- Any error messages about initialization failures

---

## Complete Wiring Diagram

```
XIAO ESP32S3          Grove-Wio-E5 Module
┌─────────┐           ┌──────────────┐
│   5V    │───────────│ (not used)   │
│   3.3V  │───────────│     VCC      │
│   GND   │───────────│     GND      │
│ D7(TX43)│───────────│  RX (input)  │
│ D6(RX44)│───────────│  TX (output) │
│         │           └──────────────┘
│         │
│   D9    │────[220Ω]─→ LED+ → LED- ──→ GND
│         │
│   D10   │───────────→ Relay Signal (IN)
│   5V    │───────────→ Relay VCC
│   GND   │───────────→ Relay GND
└─────────┘

Relay Module          Stove Control
┌─────────┐           ┌─────────────┐
│   COM   │───────────│  Control+   │
│   NO    │───────────│  Control-   │
│   NC    │  (unused) │             │
└─────────┘           └─────────────┘
```

---

## Power Recommendations

- **XIAO ESP32S3**: Can be powered via USB-C or VIN pin
- **Grove-Wio-E5**: Requires stable 3.3V (max current ~100mA)
- **Relay Module**: Check your module specs (typically 5V, ~70mA)
- **Status LED**: ~20mA through current-limiting resistor

**Total Power Budget:** ~200mA at 5V (USB power is sufficient)

---

## Safety Notes

⚠️ **Gas Stove Control**

- Test thoroughly before connecting to actual stove
- Implement proper safety timeout (default: 10 minutes)
- Consider adding manual override switch
- Follow local electrical codes
- Use properly rated relay for your application

⚠️ **Electrical Safety**

- Double-check all connections before powering on
- Use proper wire gauge for relay load
- Isolate high-voltage wiring from low-voltage circuits
- Consider using optoisolated relay module for safety

---

## Testing Procedure

1. **LoRa First**: Test LoRa communication (should see "P2P RX: STATUS_REQUEST")
2. **LED Second**: Verify LED patterns during different states
3. **Relay Last**: Test relay clicking and voltage on D10
4. **Integration**: Test complete system with temperature commands

Monitor serial output at 115200 baud for detailed diagnostics.
