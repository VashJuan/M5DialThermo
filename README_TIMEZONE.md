# Timezone Configuration

## Overview

The M5Dial thermostat now features **automatic timezone detection** with
multiple fallback layers for robust operation:

1. **Automatic Detection**: Uses IP geolocation to detect your timezone
   automatically
2. **Manual NTP Configuration**: Uses configured timezone if auto-detection
   fails
3. **Fallback Configuration**: Uses timezone stored in temps.csv if WiFi/NTP
   fails

## How It Works

### 1. **Automatic Timezone Detection (Primary Method)**

When WiFi connects successfully, the system:

- Queries WorldTimeAPI.org using your IP address
- Automatically detects your geographic timezone
- Converts to ESP32-compatible format
- Uses this for NTP synchronization

### 2. **Manual Configuration (Secondary Method)**

If automatic detection fails:

- Uses the configured timezone in the code (default: UTC-8)
- Synchronizes with NTP servers using this timezone

### 3. **Fallback Method (No Internet)**

When WiFi connection or NTP synchronization fails:

- Loads timezone from temps.csv file
- Configures local timezone without NTP sync
- Provides time display functionality (requires manual time setting)

## Editing the Fallback Timezone

### Step 1: Edit temps.csv

Open the `temps.csv` file and locate the line:

```JSON
FallbackTimezone,UTC-8
```

### Step 2: Change the Timezone

Replace `UTC-8` with your desired timezone using the standard timezone format:

#### Common Timezone Examples

- **Pacific Standard Time**: `UTC-8` (or `UTC-7` during daylight saving)
- **Mountain Standard Time**: `UTC-7` (or `UTC-6` during daylight saving)
- **Central Standard Time**: `UTC-6` (or `UTC-5` during daylight saving)
- **Eastern Standard Time**: `UTC-5` (or `UTC-4` during daylight saving)
- **Greenwich Mean Time**: `UTC+0`
- **Central European Time**: `UTC+1`
- **Japan Standard Time**: `UTC+9`

#### Advanced Timezone Strings (with Daylight Saving)

- **US Pacific**: `PST8PDT,M3.2.0,M11.1.0`
- **US Eastern**: `EST5EDT,M3.2.0,M11.1.0`
- **Central European**: `CET-1CEST,M3.5.0,M10.5.0/3`

### Step 3: Upload to Device

After editing, upload the modified `temps.csv` file to your M5Dial's SPIFFS
filesystem.

## Testing the Configuration

1. Disconnect from WiFi or disable WiFi on your router
2. Restart the M5Dial
3. Check the serial output for messages like:

   ```text
   WiFi connection failed, attempting fallback timezone setup...
   Loading fallback timezone from temps.csv
   Loaded fallback timezone: UTC-8
   ```

## Important Notes

- The fallback timezone only affects display formatting; it does not synchronize
  the actual time
- For accurate timekeeping without NTP, you may need to manually set the RTC
  time
- When WiFi/NTP is restored, the system will automatically switch back to NTP
  synchronization
- Changes to the fallback timezone take effect after the next device restart

## Troubleshooting

### If timezone fallback doesn't work

1. Verify `temps.csv` exists on the device's SPIFFS filesystem
2. Check that the line format is exactly: `FallbackTimezone,YOUR_TIMEZONE`
3. Ensure there are no extra spaces or special characters
4. Monitor serial output for error messages during startup

### Default Behavior

If the fallback timezone cannot be loaded from `temps.csv`, the system will use
`UTC-8` as the default fallback timezone.
