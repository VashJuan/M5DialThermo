# 🌐 Network & Communication Guide

## Overview

This guide covers LoRa communication setup, network configuration, and
troubleshooting for the M5Dial Smart Thermostat system.

## Communication Architecture

### System Overview

```
M5Dial (internal RTC & external temperature sensor: MCP9808 or similar)
        ↓
[LoRa P2P - transmitter side] - Lora Wio-E5-HF
        ↓
[LoRa P2P - receiver side] - Lora Wio-E5-HF
        ↓
XIAO ESP32S3 (Receiver)
        ↓
Relay Control (inside the Heater/Stove)
```

### Dual Communication Modes

The system supports two LoRa communication modes with automatic fallback:

**1. P2P Mode (Default)** - Point-to-Point **2. LoRaWAN Mode (Fallback)** -
Network Infrastructure

## P2P Mode (Point-to-Point)

### What is P2P Mode?

Direct device-to-device communication without infrastructure:

- No gateway or network server required
- Simple setup and configuration
- Lower latency
- Perfect for single home/building installations

### P2P Configuration

**RF Parameters** (in `shared/protocol_common.hpp`):

```cpp
// Default P2P Settings
const uint32_t P2P_FREQUENCY = 915000000;    // 915 MHz (US)
const uint8_t P2P_SPREADING_FACTOR = 12;     // SF12 (max range)
const uint8_t P2P_BANDWIDTH = 125;           // 125 kHz
const uint8_t P2P_CODING_RATE = 1;           // 4/5 coding rate
const uint8_t P2P_POWER = 14;                // 14 dBm
```

**Regional Frequencies:**

- **US:** 915 MHz (915000000)
- **EU:** 868 MHz (868300000)
- **Asia:** 923 MHz (923000000)
- **Australia:** 915 MHz (915000000)

### P2P Communication Flow

```
1. M5Dial: Enter TEST mode (AT+MODE=TEST)
2. M5Dial: Configure RF parameters (AT+TEST=RFCFG,...)
3. M5Dial: Send message (AT+TEST=TXLRPKT,"hex_data")
4. Receiver: Listen for packets (AT+TEST=RXLRPKT)
5. Receiver: Parse received data
6. Receiver: Send response
7. M5Dial: Receive response
```

### P2P Advantages

✅ **No infrastructure needed** - Works standalone  
✅ **Lower latency** - Typical 1-3 second response  
✅ **Simple setup** - No network configuration  
✅ **Privacy** - No data goes through external network  
✅ **Cost effective** - No gateway required

### P2P Limitations

❌ **Range** - Limited to LoRa radio range (100-300m indoor)  
❌ **Single link** - One transmitter to one receiver  
❌ **No internet** - Cannot integrate with cloud services  
❌ **Fixed frequency** - Must coordinate if multiple systems nearby

## LoRaWAN Mode (Network Infrastructure)

### What is LoRaWAN?

A protocol for long-range, low-power wireless networks:

- Uses LoRaWAN gateways
- Supports many devices
- Internet connectivity
- Cloud service integration

### LoRaWAN Network Components

```
Device (M5Dial/Receiver)
        ↓
   LoRaWAN Gateway
        ↓
  Network Server (TTN, etc.)
        ↓
  Application Server
```

### LoRaWAN Configuration

**Required Credentials:**

- **DevEUI** - Device unique identifier
- **AppEUI** - Application identifier
- **AppKey** - Application encryption key

**Obtaining Credentials:**

1. **The Things Network (TTN)** - Free:

   - Visit https://www.thethingsnetwork.org/
   - Create account and application
   - Add device (OTAA)
   - Copy AppEUI and AppKey

2. **ChirpStack** - Self-hosted:

   - Install ChirpStack server
   - Create application and device
   - Generate keys

3. **Commercial Providers:**
   - Helium Network
   - AWS IoT Core for LoRaWAN
   - Actility ThingPark

**Configuration File** (`src/secrets.h`):

```cpp
// LoRaWAN Credentials
#define LORAWAN_APP_EUI "0000000000000000"  // From network
#define LORAWAN_APP_KEY "00000000000000000000000000000000"  // From network
#define LORAWAN_REGION "US915"  // or "EU868", "AS923", etc.
```

### LoRaWAN Join Process

**OTAA (Over-The-Air Activation):**

```
1. Device: Send JOIN request with DevEUI, AppEUI
2. Network: Validate credentials
3. Network: Send JOIN accept with session keys
4. Device: Store session keys
5. Device: Ready for uplink/downlink messages
```

**Join typically takes 5-15 seconds**

### LoRaWAN Communication Flow

```
1. M5Dial: Join network (if not joined)
2. M5Dial: Send uplink message with command
3. Gateway: Forward to network server
4. Network: Route to receiver
5. Receiver: Process command
6. Receiver: Send uplink response
7. Network: Schedule downlink to M5Dial
8. M5Dial: Receive response
```

### LoRaWAN Advantages

✅ **Extended range** - Through gateway network  
✅ **Multiple devices** - Many devices per gateway  
✅ **Internet integration** - Cloud services, remote access  
✅ **Scalable** - Add devices without infrastructure changes  
✅ **Standardized** - Global standard, interoperable

### LoRaWAN Limitations

❌ **Infrastructure required** - Need gateway and network  
❌ **Higher latency** - Typical 5-15 second response  
❌ **Complexity** - More configuration steps  
❌ **Potential costs** - Gateway hardware, network fees  
❌ **Privacy** - Data passes through network server

## Automatic Mode Fallback

### How Fallback Works

```cpp
// Automatic fallback logic
1. Try P2P mode first
   ↓
   Success? → Use P2P
   ↓
   Failed? → Try LoRaWAN
   ↓
   Success? → Use LoRaWAN
   ↓
   Failed? → Return error
```

**Example Usage:**

```cpp
String response = lora.sendCommandWithFallback("STOVE_ON");
// Tries P2P first, LoRaWAN if P2P fails
```

### Mode Selection Display

M5Dial screen shows current mode:

- **"P2P"** - Using point-to-point
- **"LoRaWAN"** - Using network mode
- **"ERROR"** - Both modes failed

### Manual Mode Switching

```cpp
// Switch to specific mode if needed
lora.switchMode(LoRaCommunicationMode::P2P);
lora.switchMode(LoRaCommunicationMode::LoRaWAN);
```

## WiFi Configuration (Time Sync Only)

### Purpose

WiFi is used **only** for:

- NTP time synchronization
- Automatic timezone detection
- Initial RTC setup

**Note:** WiFi is NOT used for thermostat control communication.

### WiFi Setup

**Configuration File** (`src/secrets.h`):

```cpp
#define DEFAULT_WIFI_SSID "Your_Network_Name"
#define DEFAULT_WIFI_PASSWORD "Your_Password"
```

**Requirements:**

- 2.4 GHz WiFi network (ESP32 limitation)
- WPA2-PSK security (most compatible)
- Internet access for NTP and timezone

### WiFi Behavior

**At Startup:**

1. Connect to WiFi
2. Detect timezone via worldtimeapi.org
3. Sync time via NTP (time.nist.gov, pool.ntp.org)
4. Update RTC
5. **Disconnect WiFi** to save power

**After Initialization:**

- WiFi remains disabled
- RTC maintains time independently
- No ongoing WiFi connection needed

### Fallback Without WiFi

If WiFi fails:

- Uses fallback timezone from `temps.csv`
- Manual time setting possible
- System continues normal operation
- LoRa communication unaffected

## Network Troubleshooting

### P2P Communication Issues

**Problem: No Response from Receiver**

**Check:**

1. ✓ Both devices using P2P mode
2. ✓ Same frequency configured (915 MHz US, 868 MHz EU)
3. ✓ Within range (100-300m typical indoor)
4. ✓ Antennas attached and oriented properly
5. ✓ No metal obstructions between devices

**Solutions:**

```
• Move devices closer (test at 1m first)
• Check antenna connections
• Verify P2P_FREQUENCY matches region
• Increase spreading factor for reliability:
  P2P_SPREADING_FACTOR = 12 (max range, slower)
• Increase transmit power (within regulations):
  P2P_POWER = 14 (or up to 20 for some regions)
```

**Problem: Intermittent Communication**

**Causes:**

- Interference from other 915 MHz devices
- Marginal signal strength
- Power supply issues
- Timing problems

**Solutions:**

```
• Test at different times of day
• Change frequency slightly (within band)
• Add retry logic (already implemented)
• Check power supply stability
```

### LoRaWAN Issues

**Problem: Join Failed**

**Serial Output:**

```
Join attempt 1/3
Join command failed
All join attempts failed
```

**Check:**

1. ✓ AppEUI correct (16 hex characters)
2. ✓ AppKey correct (32 hex characters)
3. ✓ Region setting matches gateway (US915/EU868)
4. ✓ Gateway in range and online
5. ✓ Device registered on network

**Solutions:**

```
1. Verify credentials in secrets.h
2. Check network server (TTN) device status
3. Confirm gateway coverage:
   - TTN Mapper: https://ttnmapper.org/
   - ChirpStack coverage map
4. Try different location (near window)
5. Check antenna connection
```

**Problem: Join Successful but No Downlink**

**Causes:**

- Network server not routing downlinks
- Receiver not sending response
- Timing windows missed

**Solutions:**

```
• Check network server logs
• Verify receiver is joined to network
• Enable confirmed uplinks (slower but more reliable)
• Increase RX window timeout
```

### WiFi/NTP Issues

**Problem: Cannot Connect to WiFi**

**Serial Output:**

```
WiFi Connection Failed. Status: WL_CONNECT_FAILED
```

**Check:**

1. ✓ SSID correct in secrets.h
2. ✓ Password correct (case sensitive)
3. ✓ 2.4 GHz network (not 5 GHz)
4. ✓ WPA2 security (not WPA3 only)
5. ✓ No MAC address filtering

**Solutions:**

```
• Double-check credentials
• Connect to phone hotspot (test)
• Check router allows IoT devices
• Disable MAC filtering temporarily
```

**Problem: NTP Synchronization Failed**

**Serial Output:**

```
SNTP status: 0 (RESET)
Warning: SNTP not starting
```

**Check:**

1. ✓ Internet connectivity
2. ✓ UDP port 123 not blocked
3. ✓ Firewall allows NTP
4. ✓ DNS resolution working

**Solutions:**

```
• Test NTP from computer:
  Windows: w32tm /stripchart /computer:time.nist.gov
  Linux: ntpdate -q time.nist.gov

• Check firewall rules:
  Allow UDP port 123 outbound

• Try alternative DNS:
  WiFi.config(..., IPAddress(8, 8, 8, 8))

• Use fallback timezone configuration
```

**Problem: HTTP Request Failed (Error -5)**

**Serial Output:**

```
HTTP request failed: -5
Error: TCP connection failed
```

**Causes:**

- Firewall blocking HTTP (port 80)
- ISP blocking worldtimeapi.org
- NAT/router issues
- DNS resolution problems

**Solutions:**

```
1. Test from computer:
   curl http://worldtimeapi.org/api/ip

2. Check router firewall settings

3. Try mobile hotspot (isolate network issue)

4. Use fallback timezone:
   Edit temps.csv:
   FallbackTimezone,PST8PDT,M3.2.0,M11.1.0
```

## Communication Range Optimization

### Improving P2P Range

**Hardware:**

- ✓ Use better antennas (3-5 dBi gain)
- ✓ Mount units higher
- ✓ Ensure antennas vertical
- ✓ Add antenna extensions if needed

**Software:**

- ✓ Increase spreading factor (SF12 max)
- ✓ Increase transmit power (within regulations)
- ✓ Use lower bandwidth (better range, slower)
- ✓ Enable retry logic (already implemented)

**Configuration for Maximum Range:**

```cpp
const uint8_t P2P_SPREADING_FACTOR = 12;  // Slowest, longest range
const uint8_t P2P_BANDWIDTH = 125;        // Standard
const uint8_t P2P_POWER = 14;             // US limit (20 for some regions)
```

### Range Testing

**Test Procedure:**

1. Start with devices 1 meter apart
2. Verify communication works (100% success)
3. Double distance each test
4. Note when reliability drops below 90%
5. That's your reliable range

**Expected Ranges:**

- **Indoor residential:** 100-300 meters
- **Indoor commercial:** 50-150 meters
- **Outdoor line-of-sight:** 1-2 km
- **Through walls:** 50-100 meters

## Security Considerations

### P2P Mode Security

**Current Security:**

- Commands sent in plaintext
- No encryption by default
- Anyone with same frequency can intercept

**Improvements Possible:**

- Add message encryption
- Use rolling codes
- Implement message authentication

### LoRaWAN Security

**Built-in Security:**

- ✅ AES-128 encryption
- ✅ Network session keys
- ✅ Application session keys
- ✅ Unique device addresses

**Best Practices:**

- Use OTAA (not ABP)
- Keep AppKey secret
- Rotate keys periodically
- Use confirmed messages for critical commands

## Diagnostic Tools

### Serial Monitor

**Enable Debug Output:**

```cpp
Serial.begin(115200);  // M5Dial and XIAO
```

**Key Messages:**

```
SUCCESS! Module responding at 19200 baud  // Good
TX: AT+TEST=TXLRPKT,"..."                 // P2P transmit
RX: +TEST: RX "..."                       // P2P receive
+JOIN: Network joined                      // LoRaWAN joined
```

### Signal Quality Monitoring

**Automatic Monitoring:**

- Reports RSSI and SNR every 5 minutes
- Shows in serial monitor
- Helps diagnose range issues

**RSSI (Signal Strength):**

- **> -60 dBm:** Excellent
- **-60 to -80 dBm:** Good
- **-80 to -100 dBm:** Fair
- **< -100 dBm:** Poor

**SNR (Signal-to-Noise Ratio):**

- **> 10 dB:** Excellent
- **5 to 10 dB:** Good
- **0 to 5 dB:** Fair
- **< 0 dB:** Poor (errors likely)

### Network Testing Commands

**P2P Test:**

```cpp
lora.sendP2PMessage("TEST");  // Send test message
```

**LoRaWAN Test:**

```cpp
lora.ping();  // Send ping, expect pong
```

**Mode Check:**

```cpp
LoRaCommunicationMode mode = lora.getCurrentMode();
// Returns P2P or LoRaWAN
```

## Best Practices

### Installation

1. ✅ Test communication on bench first
2. ✅ Verify range at installation site
3. ✅ Mount antennas vertically
4. ✅ Keep antennas away from metal
5. ✅ Document configuration settings

### Operation

1. ✅ Monitor signal quality periodically
2. ✅ Check for firmware updates
3. ✅ Log communication errors
4. ✅ Test safety timeout monthly
5. ✅ Verify backup mode works

### Troubleshooting

1. ✅ Start with serial monitor output
2. ✅ Test at short range first
3. ✅ Verify both units powered on
4. ✅ Check baud rate configuration
5. ✅ Test each mode independently

---

**Version:** 2.0.0  
**Last Updated:** January 2026  
**Author:** John Cornelison (john@vashonSoftware.com)
