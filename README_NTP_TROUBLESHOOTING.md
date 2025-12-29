# NTP Synchronization Troubleshooting Guide

This guide helps diagnose and fix NTP synchronization issues with your M5Dial
Thermostat.

## Common Issues and Solutions

### 1. WiFi Connection Problems

**Symptoms:**

- "WiFi Connection Failed" in serial output
- Device shows "Initializing clock..." indefinitely

**Solutions:**

1. **Check WiFi Credentials:**

   - Verify SSID and password in `src/secrets.h`
   - Ensure no special characters that might cause issues
   - Check for case sensitivity

2. **Network Range:**

   - Move device closer to WiFi router
   - Check signal strength (should be > -70 dBm)
   - Ensure 2.4GHz network is available (M5Dial doesn't support 5GHz)

3. **Router Configuration:**
   - Disable AP isolation if enabled
   - Ensure DHCP is enabled
   - Check for MAC address filtering
   - Verify firewall settings allow new device connections

### 2. NTP Synchronization Failures

**Symptoms:**

- "NTP Synchronization Failed (SNTP enabled)" in serial output
- WiFi connects but time doesn't sync

**Solutions:**

1. **Firewall Issues:**

   ```
   Port 123 (NTP) must be open for outbound UDP traffic
   ```

2. **DNS Problems:**

   - Check if DNS servers are accessible
   - The code automatically uses Google DNS (8.8.8.8) and Cloudflare DNS
     (1.1.1.1)

3. **NTP Server Issues:**

   - Default servers: 0.pool.ntp.org, 1.pool.ntp.org, 2.pool.ntp.org
   - Try changing to regional servers in `rtc.cpp`:
     ```cpp
     static const char* DEFAULT_NTP_SERVER1 = "time.nist.gov";
     static const char* DEFAULT_NTP_SERVER2 = "pool.ntp.org";
     ```

4. **Network Congestion:**
   - Increase timeout values if network is slow
   - Try during off-peak hours

### 3. Timezone Configuration Issues

**Symptoms:**

- Time displays but is wrong by several hours
- "Time unavailable (no timezone)" messages

**Solutions:**

1. **Check Timezone Format:**

   - Current setting in `data/temps.csv`: `PST8PDT,M3.2.0,M11.1.0`
   - This is Pacific Time with automatic daylight saving
   - For other timezones:
     - Eastern: `EST5EDT,M3.2.0,M11.1.0`
     - Central: `CST6CDT,M3.2.0,M11.1.0`
     - Mountain: `MST7MDT,M3.2.0,M11.1.0`
     - UTC: `UTC0`

2. **Manual Timezone Update:**
   - Edit the `FallbackTimezone` line in `data/temps.csv`
   - The format is: `FallbackTimezone,<timezone_string>`

### 4. Watchdog Timer Issues

**Symptoms:**

- Device reboots during setup
- Timeout messages in serial output

**Solutions:**

1. **Current timeout is set to 20 seconds** in `platformio.ini`
2. **Reduce WiFi timeout** if network is consistently slow
3. **Check for interference** that might slow down network operations

## Diagnostic Commands

### Serial Monitor Output Analysis

Look for these key messages:

#### Successful NTP Sync:

```
WiFi Connected.
IP address: 192.168.1.100
DNS test successful: google.com -> 142.250.191.110
NTP Connected via SNTP.
RTC hardware updated: 2025/12/29 (Sun) 18:30:15 UTC
```

#### Failed NTP Sync:

```
WiFi Connected.
DNS test failed: error -5
NTP Synchronization Failed (SNTP enabled). Status: 2
Using fallback timezone...
```

#### Fallback Mode:

```
WiFi connection failed, using fallback timezone...
Loaded fallback timezone: 'PST8PDT,M3.2.0,M11.1.0'
Timezone configured successfully: PST8PDT,M3.2.0,M11.1.0
```

## Fallback Mode

If NTP sync fails, the device will operate using:

1. **Fallback timezone** from `temps.csv`
2. **No automatic time sync** - time may drift
3. **Manual time adjustment** may be needed

The device will automatically retry NTP synchronization:

- Every 30 seconds for the first 3 attempts
- Then stops retrying to avoid wasting battery

## Network Requirements

### Ports Required:

- **Port 53 (DNS):** UDP outbound
- **Port 123 (NTP):** UDP outbound
- **Port 80 (HTTP):** TCP outbound (for timezone detection)

### Bandwidth:

- Very minimal - only a few KB for NTP sync
- No streaming or large data transfers

## Advanced Configuration

### Custom NTP Servers

Edit `rtc.cpp` to use custom NTP servers:

```cpp
static const char* DEFAULT_NTP_SERVER1 = "your.ntp.server.com";
static const char* DEFAULT_NTP_SERVER2 = "backup.ntp.server.com";
```

### Timeout Adjustments

For slower networks, increase timeouts in `rtc.cpp`:

```cpp
int maxAttempts = 60; // WiFi: 60 * 250ms = 15 seconds
int maxAttempts = 40; // NTP: 40 * 250ms = 10 seconds
```

## Hardware Considerations

### M5Dial Specific Issues:

1. **Antenna placement** - keep away from metal objects
2. **Power supply stability** - ensure adequate USB power
3. **Temperature** - avoid overheating which can affect WiFi

### ESP32-S3 WiFi:

- Only supports 2.4GHz networks
- Maximum range depends on environment
- Performance may vary with ESP32 Arduino framework version

## Getting Help

If you continue to experience issues:

1. **Enable verbose logging** by setting Serial monitor to 9600 baud
2. **Check the full serial output** during startup
3. **Verify network configuration** using another device on the same network
4. **Test with a mobile hotspot** to isolate router issues

## Version Information

This troubleshooting guide is for:

- **M5Dial Thermostat** v2.0.0
- **ESP32 Arduino Framework** (via PlatformIO)
- **M5Unified** v0.1.13+
