# Network Connectivity Troubleshooting Guide

This guide addresses common network connectivity issues experienced with the
M5Stack Dial thermostat project, particularly related to NTP synchronization and
timezone detection.

## Common Issues and Solutions

### 1. HTTP Request Failures (Error -5: TCP Connection Failed)

**Symptoms:**

```
DNS resolution successful: worldtimeapi.org -> 213.188.196.246
HTTP request failed: -5
Error: TCP connection failed or network unreachable
```

**Potential Causes:**

- Firewall blocking HTTP traffic from ESP32
- NAT/Router configuration issues
- ISP blocking specific websites
- Network congestion or instability

**Solutions:**

1. **Check Router/Firewall Settings:**

   - Ensure HTTP traffic (port 80) is allowed for IoT devices
   - Check if your router has IoT device restrictions
   - Temporarily disable firewall to test connectivity

2. **Test Alternative Networks:**

   - Try connecting to a mobile hotspot to isolate network issues
   - Test on a different WiFi network

3. **Router Configuration:**
   - Enable UPnP if available
   - Check DNS settings (code now uses 75.75.75.75, 75.75.76.76)
   - Update router firmware

### 2. SNTP Status 0 (RESET) - NTP Not Starting

**Symptoms:**

```
Initial SNTP status: 0
Warning: SNTP not starting - may indicate network issues
```

**Potential Causes:**

- Firewall blocking NTP traffic (port 123 UDP)
- NTP servers unreachable
- Network latency issues

**Solutions:**

1. **Check NTP Ports:**

   - Ensure UDP port 123 is open for outbound connections
   - Some corporate networks block NTP traffic

2. **Alternative NTP Servers:** The code now includes fallback servers:

   - Primary: time.nist.gov, pool.ntp.org, 0.pool.ntp.org
   - Fallback: time.google.com, time.cloudflare.com, time.nist.gov

3. **Router NTP Settings:**
   - Check if router has built-in NTP blocking
   - Some routers redirect NTP requests to local NTP server

### 3. WiFi Connection Issues

**Symptoms:**

```
WiFi Connection Failed. Status: [error_code]
```

**Status Code Meanings:**

- `WL_NO_SSID_AVAIL` (1): Network not found
- `WL_CONNECT_FAILED` (4): Wrong password or security settings
- `WL_CONNECTION_LOST` (5): Connection dropped
- `WL_DISCONNECTED` (6): Not connected

**Solutions:**

1. **Verify Credentials:**

   - Double-check SSID and password in `secrets.h`
   - Ensure WiFi network is 2.4GHz (ESP32-S3 limitation)
   - Check for special characters in password

2. **Network Settings:**

   - Try WPA2-PSK security (most compatible)
   - Avoid WPA3-only networks
   - Check if MAC filtering is enabled

3. **Signal Strength:**
   - Move device closer to router for testing
   - Check for interference from other 2.4GHz devices

### 4. DNS Resolution Issues

**Symptoms:**

```
DNS test failed: error [code]
NTP server DNS resolution failed
```

**Solutions:**

1. **DNS Server Configuration:** The code now uses Cloudflare DNS (75.75.75.75,
   75.75.76.76)

   - Alternative: Google DNS (8.8.8.8, 8.8.4.4)
   - Try your ISP's DNS servers

2. **Router DNS Settings:**
   - Check if router is blocking external DNS
   - Ensure DNS forwarding is enabled

## Network Diagnostics

### Built-in Diagnostics

The improved code includes several diagnostic features:

1. **Enhanced Error Reporting:**

   - Detailed HTTP error codes with explanations
   - WiFi status codes with meanings
   - DNS resolution testing

2. **Multiple Connection Attempts:**

   - Direct IP connection fallback
   - Alternative NTP servers
   - Extended timeout handling

3. **Robust Fallback:**
   - Uses fallback timezone from `temps.csv`
   - Continues operation without NTP sync
   - Manual time adjustment possible

### Testing Network Connectivity

1. **Basic Connectivity Test:**

   ```
   ping 8.8.8.8
   ping google.com
   ```

2. **NTP Test (from computer):**

   ```
   w32tm /stripchart /computer:time.nist.gov
   # or on Linux/Mac:
   ntpdate -q time.nist.gov
   ```

3. **HTTP Test:**
   ```
   curl -v http://worldtimeapi.org/api/ip
   ```

## Configuration Options

### 1. Alternative NTP Servers

Edit the NTP server configuration in the code:

```cpp
const char* DEFAULT_NTP_SERVER1 = "your.ntp.server";
const char* DEFAULT_NTP_SERVER2 = "pool.ntp.org";
const char* DEFAULT_NTP_SERVER3 = "time.google.com";
```

### 2. Fallback Timezone Configuration

Update `data/temps.csv` with your timezone:

```csv
# Fallback timezone for when NTP/WiFi is not available
FallbackTimezone,PST8PDT,M3.2.0,M11.1.0
```

Common timezone strings:

- Pacific: `PST8PDT,M3.2.0,M11.1.0`
- Eastern: `EST5EDT,M3.2.0,M11.1.0`
- Central: `CST6CDT,M3.2.0,M11.1.0`
- Mountain: `MST7MDT,M3.2.0,M11.1.0`
- UTC: `UTC0`

### 3. WiFi Configuration

Ensure your `src/secrets.h` file contains:

```cpp
#define DEFAULT_WIFI_SSID "Your_Network_Name"
#define DEFAULT_WIFI_PASSWORD "Your_Network_Password"
```

## Troubleshooting Steps

### Step 1: Check Basic Connectivity

1. Verify device connects to WiFi
2. Check signal strength in serial monitor
3. Confirm DNS resolution works

### Step 2: Test Network Restrictions

1. Try connecting to mobile hotspot
2. Test on different network
3. Check router settings for IoT restrictions

### Step 3: Verify Time Services

1. Test NTP access from another device
2. Check if corporate firewall blocks time services
3. Try alternative NTP servers

### Step 4: Fallback Configuration

1. Ensure fallback timezone is set correctly
2. Verify system continues operating without NTP
3. Consider manual time adjustment if needed

## Advanced Solutions

### 1. Custom DNS Configuration

If your network requires specific DNS servers:

```cpp
// In connectToWiFi() function
WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE,
           IPAddress(192, 168, 1, 1),  // Router DNS
           IPAddress(8, 8, 8, 8));     // Backup DNS
```

### 2. Proxy Configuration

For networks requiring HTTP proxy (advanced):

- This would require additional HTTPClient configuration
- Not currently implemented but can be added if needed

### 3. Static IP Configuration

For networks requiring static IP:

```cpp
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
WiFi.config(local_IP, gateway, subnet);
```

## Expected Behavior

### Successful Operation:

```
WiFi Connected.
IP address: 192.168.1.xxx
DNS resolution successful: worldtimeapi.org -> xxx.xxx.xxx.xxx
Timezone detection successful via direct IP
Using NTP servers: time.nist.gov, pool.ntp.org, 0.pool.ntp.org
SNTP status: 1 (COMPLETED)
RTC hardware updated: 2025/12/29 (Mon) 21:19:12 UTC
```

### Fallback Mode (Network Issues):

```
WiFi connection failed, using fallback timezone...
Loaded fallback timezone: 'PST8PDT,M3.2.0,M11.1.0'
Timezone configured successfully: PST8PDT,M3.2.0,M11.1.0
Note: Time will not be synchronized with NTP servers
Fallback timezone setup complete - WiFi disabled
```

## Getting Help

If issues persist after trying these solutions:

1. **Capture Serial Output:** Full serial monitor output during startup
2. **Network Details:** Router model, ISP, network configuration
3. **Test Results:** Results from connectivity tests above

The system is designed to be resilient - it will continue operating even if
network synchronization fails, using the fallback timezone configuration.
