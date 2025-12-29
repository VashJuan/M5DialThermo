# Power Management and Energy Optimization

## Overview

The M5Dial Thermostat implements intelligent power management to maximize
battery life while maintaining responsive temperature monitoring. The system
operates in multiple power modes with automatic transitions based on user
activity and system requirements.

## Power Modes

### 1. Active Mode (High Performance)

- **Trigger:** User interaction (button press, dial movement) or system startup
- **Duration:** Active for 3 seconds after last user interaction
- **CPU Frequency:** 80MHz (normal power saving frequency)
- **Temperature Polling:** Every 5 seconds for responsive updates
- **Sensor State:** Always awake and ready
- **Display Updates:** Every 2 seconds with full color display
- **Power Consumption:** ~50-70mA (estimated)

### 2. Power Save Mode (Reduced Performance)

- **Trigger:** 3+ seconds of inactivity
- **CPU Frequency:** 40MHz (significantly reduced)
- **Temperature Polling:** Every 2 minutes (120 seconds)
- **Sensor State:** Sleep between readings, wake for polling
- **Display Updates:** Every 10 seconds with reduced color intensity
- **Power Consumption:** ~20-30mA (estimated)

### 3. Deep Power Save Mode (Maximum Efficiency)

- **Trigger:** Extended inactivity in power save mode
- **CPU Frequency:** 40MHz (maintained)
- **Temperature Polling:** Every 2 minutes (maintained)
- **Sensor State:** Full shutdown between readings
- **Display Updates:** Shows cached data with timestamps
- **Sleep Duration:** 500ms between main loop cycles
- **Power Consumption:** ~10-15mA (estimated)

## Temperature Sensor Power Management

### MCP9808 Power States

The Adafruit MCP9808 temperature sensor supports two primary power states:

1. **Active State (~200μA)**

   - Full operational capability
   - Ready for immediate temperature readings
   - Used during active mode and reading periods

2. **Shutdown State (~0.1μA)**
   - Ultra-low power consumption
   - 10ms wake-up time required before reading
   - Used during extended idle periods

### Polling Strategy

```cpp
// Adaptive polling intervals
const unsigned long TEMP_POLL_ACTIVE_INTERVAL = 5 * 1000;    // 5 seconds (active)
const unsigned long TEMP_POLL_INTERVAL = 2 * 60 * 1000;      // 2 minutes (power save)

// Intelligent sensor management
if (timeForTempPoll && !tempSensor.getAwakeStatus()) {
    tempSensor.wakeUp();
    delay(10); // Stabilization time
}

// Automatic sleep after reading in power save mode
if (powerSaveMode && tempSensor.getAwakeStatus()) {
    tempSensor.shutdown();
}
```

## Cached Temperature System

### Temperature Caching Benefits

- **Power Efficiency:** Avoids unnecessary sensor wake-ups
- **Responsive Display:** Shows last known temperature immediately
- **Aging Information:** Displays time since last actual reading
- **Graceful Degradation:** System remains functional even with sensor issues

### Implementation

```cpp
class TemperatureSensor {
private:
    float lastTemperatureC;        // Cached Celsius reading
    float lastTemperatureF;        // Cached Fahrenheit reading
    unsigned long lastReadTime;    // Timestamp of last sensor read

public:
    float getLastTemperatureF() const;     // Get cached Fahrenheit
    float getLastTemperatureC() const;     // Get cached Celsius
    unsigned long getLastReadTime() const; // Get read timestamp
};
```

### Display Integration

During power save mode, the display shows:

- Cached temperature value in gray color
- Time elapsed since last reading (e.g., "72.5 F (3m ago)")
- Power save mode indicator in status area

## CPU Frequency Scaling

### Frequency Management

The ESP32-S3 CPU frequency is dynamically adjusted based on system activity:

```cpp
// Active mode: Normal power saving frequency
setCpuFrequencyMhz(80);

// Power save mode: Reduced frequency
setCpuFrequencyMhz(40);
```

### Performance Impact

- **80MHz:** Full responsiveness for user interactions
- **40MHz:** Sufficient for background monitoring and display updates
- **Power Savings:** ~50% reduction in CPU power consumption at 40MHz

## Activity Detection and Timeouts

### Activity Tracking

```cpp
static uint32_t lastActivityTime = millis();
static const int activityTimeout = 3000; // 3 seconds

// Activity triggers (reset timeout)
void handleButtonPress() {
    lastActivityTime = millis();
    recentActivity = true;
}

void handleEncoderChange() {
    lastActivityTime = millis();
    recentActivity = true;
}
```

### Power Mode Transitions

```cpp
bool isInactive = (millis() - lastActivityTime > activityTimeout);

if (isInactive && !powerSaveMode) {
    // Enter power save mode
    setCpuFrequencyMhz(40);
    powerSaveMode = true;
} else if (!isInactive && powerSaveMode) {
    // Exit power save mode
    setCpuFrequencyMhz(80);
    powerSaveMode = false;
}
```

## Watchdog Timer Management

### Watchdog Configuration

```cpp
esp_task_wdt_init(10, true); // 10 second timeout, panic on timeout
esp_task_wdt_add(NULL);      // Add current task to watchdog
```

### Regular Feeding

```cpp
void loop() {
    yield(); // Feed at start of loop

    // ... main loop processing ...

    esp_task_wdt_reset(); // Feed at end of loop
}
```

## Power Consumption Estimates

| Mode       | CPU  | Sensor | Display | WiFi | Total Est. |
| ---------- | ---- | ------ | ------- | ---- | ---------- |
| Active     | 40mA | 0.2mA  | 15mA    | 5mA  | ~60mA      |
| Power Save | 20mA | 0.1mA  | 8mA     | 0mA  | ~28mA      |
| Deep Save  | 20mA | 0.1mA  | 5mA     | 0mA  | ~25mA      |

_Note: These are rough estimates and actual consumption may vary based on
display brightness, WiFi usage, and other factors._

## Battery Life Estimates

Assuming typical usage patterns:

### M5Dial Internal Battery (likely ~500-1000mAh)

- **Active use (10% of time):** 8-12 hours
- **Mixed use (power save 80% of time):** 15-25 hours
- **Background monitoring (deep save 95% of time):** 20-35 hours

### External Battery Pack (10,000mAh)

- **Continuous active mode:** ~150-170 hours (6-7 days)
- **Typical mixed usage:** ~300-400 hours (12-17 days)
- **Background monitoring:** ~400-500 hours (17-21 days)

## Optimization Recommendations

### For Maximum Battery Life

1. Reduce display brightness if possible
2. Increase temperature polling interval to 5+ minutes
3. Implement motion-based wake-up instead of periodic polling
4. Use WiFi only for initial time sync, then disable

### For Maximum Responsiveness

1. Reduce activity timeout to 1-2 seconds
2. Keep sensor always awake during active periods
3. Increase display refresh rate during active mode
4. Maintain higher CPU frequency longer

## Debugging Power Issues

### Serial Output Monitoring

The system provides detailed power management logging:

```
Entering power save mode (CPU 40MHz, periodic temp polling)
Temperature sensor woken for periodic poll
Periodic temperature poll: 72.3°F (interval: 120s)
Temperature sensor shutdown after poll - next wake in 2 minutes
Entering deep power save mode - temperature polling every 2 minutes
```

### Power State Indicators

- Display color coding (gray = cached, normal colors = fresh)
- Status area messages ("power save", timestamps)
- Serial debug output for state transitions

## Future Enhancements

### Potential Power Optimizations

1. **Motion Detection:** Use accelerometer to detect when device is being
   handled
2. **Light Sleep:** Implement ESP32 light sleep between main loop cycles
3. **Dynamic Polling:** Adjust polling frequency based on temperature change
   rate
4. **Display Sleep:** Turn off display completely during extended idle periods
5. **WiFi Power Management:** Implement periodic WiFi wake-up for data
   transmission

### Advanced Features

1. **Battery Monitoring:** Track and display battery level and estimated
   remaining time
2. **Power Profiles:** User-configurable power management profiles
3. **Predictive Wake-up:** Learn usage patterns and pre-emptively wake before
   expected use
4. **Temperature Trend Analysis:** Reduce polling when temperature is stable
