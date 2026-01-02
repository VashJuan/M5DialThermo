# 🌡️ M5Dial Smart Thermostat - User Guide

## Welcome

Your M5Dial Smart Thermostat provides precision temperature control with
wireless stove/heater control. This guide covers basic operation and features.

## Quick Overview

**Main Unit (M5Dial):**

- Temperature display and monitoring
- Rotary dial for temperature adjustment
- Wireless LoRa communication to receiver
- Battery or USB powered

**Receiver Unit (XIAO ESP32S3):**

- Receives commands from M5Dial
- Controls stove/heater relay
- Status LED indicators
- Automatic safety shutoff

## Basic Operation

### 1. Temperature Display

**Main Screen Shows:**

- **Current Temperature** - Large display in Fahrenheit
- **Target Temperature** - Scheduled or adjusted setpoint
- **Time** - Current time from RTC
- **Heating Status** - ON/OFF indicator
- **Communication Mode** - P2P or LoRaWAN

### 2. Adjusting Temperature

**Using the Dial:**

1. **Rotate clockwise** - Increase target temperature
2. **Rotate counter-clockwise** - Decrease target temperature
3. Changes apply immediately
4. Target temperature shown on screen

**Manual Override:**

- Dial adjustments override scheduled temperature
- Override remains until next schedule change
- Schedule resumes at next time period

### 3. Heating Control

**Automatic Operation:**

- System compares current vs target temperature
- Heater turns ON when below target
- Heater turns OFF when at or above target
- Commands sent wirelessly to receiver

**Manual Control:**

- Press button to force heater ON/OFF (if implemented)
- Safety timeout ensures heater turns OFF after 10 minutes

## Receiver Status LED Codes

The receiver unit has an LED that indicates system status:

| LED Pattern                 | Status       | Meaning                   |
| --------------------------- | ------------ | ------------------------- |
| 💡 **Slow pulse**           | Initializing | System starting up        |
| 🔦 **Brief flash every 2s** | Ready        | Waiting for commands      |
| ⚡ **Fast blink**           | Active       | Receiving data            |
| 🟢 **Solid ON**             | Heating      | Stove/heater is ON        |
| ⚪ **Solid OFF**            | Standby      | Stove/heater is OFF       |
| 🚨 **Fast flash**           | Safety       | Timeout - heater auto-OFF |
| 🆘 **SOS pattern**          | Error        | System error occurred     |

## Temperature Schedule

### How Scheduling Works

Temperature automatically adjusts throughout the day based on your schedule:

**Example Default Schedule:**

- **1 AM-8 AM:** Lower temperature (sleep mode)
- **8 AM-5 PM:** Normal comfort temperature
- **5 PM-10 PM:** Higher temperature (evening comfort)
- **10 PM-1 AM:** Reduced temperature (wind-down)

### Understanding the Schedule Display

The M5Dial shows:

- **Base Temperature:** Your baseline comfort temperature
- **Current Offset:** How much above/below base right now
- **Actual Target:** Base + Offset = Current target

**Example:**

```
Base: 70°F
Offset: -12°F (nighttime reduction)
Target: 58°F (sleep temperature)
```

## Power & Battery

### Battery Life

**Typical Runtime:**

- **Active use:** 24-48 hours
- **Power save mode:** Up to 72 hours
- **USB powered:** Continuous operation

### Power Saving Features

**Automatic Power Management:**

1. **Active Mode** - 3 seconds after any interaction

   - Screen bright, updates every 2 seconds
   - Temperature checked every 5 seconds

2. **Power Save Mode** - After 3+ seconds idle

   - Screen dimmer, updates every 10 seconds
   - Temperature checked every 2 minutes

3. **Deep Save Mode** - Extended inactivity
   - Minimal updates, cached temperature shown
   - "Last reading X minutes ago" indicator

### Charging

- Connect USB-C cable
- Charging indicator shown (if battery equipped)
- Can operate while charging

## Communication Modes

### P2P Mode (Default)

**Advantages:**

- ✅ No network infrastructure needed
- ✅ Direct device-to-device communication
- ✅ Faster response time
- ✅ Works anywhere, no internet needed

**Best For:**

- Single home installations
- Local control without network

### LoRaWAN Mode (Fallback)

**Advantages:**

- ✅ Greater range through gateways
- ✅ Can integrate with smart home systems
- ✅ Multiple device support

**Note:** System automatically uses P2P first, falls back to LoRaWAN if P2P
fails.

## Safety Features

### Automatic Safeguards

1. **10-Minute Timeout**

   - If no communication for 10 minutes
   - Receiver automatically turns heater OFF
   - Prevents heater staying on if M5Dial fails

2. **Startup Safety**

   - Receiver always starts with heater OFF
   - Requires active command to turn ON

3. **Watchdog Protection**

   - System resets if software hangs
   - Ensures reliable operation

4. **State Verification**
   - Receiver confirms heater state changes
   - Errors logged if state doesn't match command

## Troubleshooting

### No Heating Response

**Check:**

- ✓ M5Dial shows "SENT" or "ACK" after command
- ✓ Receiver LED indicates it received command
- ✓ Both units powered on
- ✓ Within LoRa communication range

**Solutions:**

1. Move devices closer together
2. Check receiver power connection
3. Restart both units

### Temperature Not Updating

**Check:**

- ✓ "Last reading" timestamp on screen
- ✓ If >5 minutes old, may be in power save

**Solutions:**

1. Rotate dial to wake system
2. Press button to activate
3. Check temperature sensor connection

### Display Issues

**Check:**

- ✓ Screen brightness setting
- ✓ Battery level
- ✓ Power save mode active

**Solutions:**

1. Interact with dial to wake display
2. Connect to USB power
3. Adjust brightness in settings (if available)

### Communication Lost

**Symptoms:**

- Commands not acknowledged
- Receiver LED not responding
- Status shows communication errors

**Solutions:**

1. Check distance between units
2. Verify antennas attached to both modules
3. Restart both transmitter and receiver
4. Check for interference (metal objects, walls)

## Maintenance

### Weekly

- ✓ Check battery level
- ✓ Verify heater responding to commands
- ✓ Clean display if dusty

### Monthly

- ✓ Review temperature schedule effectiveness
- ✓ Check communication reliability
- ✓ Verify safety timeout working

### As Needed

- ✓ Update firmware when available
- ✓ Adjust schedule for season changes
- ✓ Replace batteries if low

## Tips for Best Performance

### Placement

**M5Dial:**

- Place at typical room height (4-5 feet)
- Away from direct heat sources
- Away from drafts or windows
- Clear line of sight to receiver (if possible)

**Receiver:**

- Near controlled heater
- Antenna oriented for best signal
- Accessible for LED status viewing

### Temperature Accuracy

- Allow 30 minutes for readings to stabilize after startup
- Avoid placing near heat vents or cold windows
- Keep sensor clean and unobstructed

### Communication Range

- Typical range: 100-300 meters indoors
- Walls and metal reduce range
- Elevate both units if possible
- Use higher mounting if range issues persist

## Support & More Information

For technical details, advanced configuration, and troubleshooting:

- **Hardware Guide:** Pin connections and technical specifications
- **Network Guide:** Communication setup and troubleshooting
- **Developer Guide:** Building and customizing firmware

---

**Version:** 2.0.0  
**Author:** John Cornelison (john@vashonSoftware.com)  
**License:** MIT

_Built for efficient, safe home heating control_ 🏠🔥
