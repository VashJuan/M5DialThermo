# LoRa Baud Rate Configuration Guide

## Overview

Both the **receiver** (XIAO ESP32S3) and **transmitter** (M5Dial) now support
configurable baud rate settings for the Grove-Wio-E5 LoRa module communication.
This allows you to:

1. **Skip the automatic baud rate search** for faster, more predictable
   initialization
2. **Set a fixed baud rate** (default: 9600) for consistent communication
3. **Configure patient connection timeouts** (default: 60 seconds) to allow
   slower devices to initialize

## Configuration Flags

### Receiver Configuration

**File:** `receiver/src/lora_receiver.hpp`

```cpp
#define LORA_DISABLE_BAUD_SEARCH false  // Set to true to skip baud rate search
#define LORA_FIXED_BAUD_RATE 9600       // Fixed baud rate when search is disabled
#define LORA_INIT_TIMEOUT_MS 60000      // Wait up to 60 seconds for M5Dial
```

### Transmitter Configuration

**File:** `src/lora_transmitter.hpp`

```cpp
#define LORA_TX_DISABLE_BAUD_SEARCH false  // Set to true to skip baud rate search
#define LORA_TX_FIXED_BAUD_RATE 9600       // Fixed baud rate when search is disabled
#define LORA_TX_INIT_TIMEOUT_MS 60000      // Allow up to 60 seconds for connection
```

## Use Cases

### Scenario 1: Fast Connection with Known Baud Rate

If you know both your LoRa modules are configured for 9600 baud:

**Receiver:**

```cpp
#define LORA_DISABLE_BAUD_SEARCH true
#define LORA_FIXED_BAUD_RATE 9600
#define LORA_INIT_TIMEOUT_MS 60000
```

**Transmitter:**

```cpp
#define LORA_TX_DISABLE_BAUD_SEARCH true
#define LORA_TX_FIXED_BAUD_RATE 9600
#define LORA_TX_INIT_TIMEOUT_MS 60000
```

**Benefits:**

- No time wasted trying wrong baud rates
- Predictable initialization time
- Receiver will wait patiently (60 seconds) for M5Dial to power up

### Scenario 2: Slow M5Dial Startup (Default)

If your M5Dial takes a long time to initialize:

**Keep defaults:**

- `LORA_DISABLE_BAUD_SEARCH false` - tries multiple baud rates
- `LORA_INIT_TIMEOUT_MS 60000` - waits up to 60 seconds
- Receiver will keep attempting connection while M5Dial initializes

### Scenario 3: Unknown Baud Rate

If you're unsure what baud rate your modules use:

**Keep defaults:**

```cpp
#define LORA_DISABLE_BAUD_SEARCH false  // Will try 19200, 9600, 115200
```

The system will automatically detect the correct baud rate.

## Initialization Behavior

### With Baud Search DISABLED (Fixed 9600)

**Receiver:**

```
Waiting for Grove-Wio-E5 and M5Dial to power up and stabilize...
Initialization timeout: 60 seconds
Using fixed baud rate: 9600 (baud search disabled)
Connection attempt 1 (elapsed: 3000 ms)...
  No response - waiting for M5Dial...
Connection attempt 2 (elapsed: 5000 ms)...
  No response - waiting for M5Dial...
...
Connection attempt 15 (elapsed: 32000 ms)...
SUCCESS! Module responding at 9600 baud
```

**Transmitter:**

```
Setting up LoRa transmitter on pins RX:1, TX:2
Initializing LoRa module - patient connection mode enabled
Initialization timeout: 60 seconds
Using fixed baud rate: 9600 (baud search disabled)
Connection attempt 1 (elapsed: 2000 ms)...
SUCCESS! Module responding at 9600 baud
```

### With Baud Search ENABLED (Default)

The system tries 19200, 9600, and 115200 baud rates sequentially, with 5
attempts per baud rate, up to the 60-second timeout.

## Timeout Configuration

### Increasing Timeout (Very Slow M5Dial)

If 60 seconds isn't enough:

```cpp
#define LORA_INIT_TIMEOUT_MS 120000  // 2 minutes
```

### Decreasing Timeout (Fast Startup Expected)

If you want faster failure detection:

```cpp
#define LORA_INIT_TIMEOUT_MS 30000  // 30 seconds
```

## Troubleshooting

### Issue: "Could not communicate with module"

**Solutions:**

1. Verify both devices use the same baud rate
2. Check physical connections (RX/TX not swapped)
3. Ensure 3.3V power is stable
4. Try increasing `LORA_INIT_TIMEOUT_MS`
5. If using fixed baud, verify modules are actually configured for that baud

### Issue: M5Dial seems to work but receiver times out

**Solutions:**

1. Increase `LORA_INIT_TIMEOUT_MS` to 90000 or 120000
2. Check that M5Dial serial initialization happens before LoRa setup
3. Verify M5Dial isn't in a boot loop or sleep mode

### Issue: Connection works sometimes but not others

**Solutions:**

1. Enable fixed baud rate mode on both devices
2. Add delays in M5Dial setup before initializing LoRa
3. Check for power supply instability

## Recommendations

### For Production Use

```cpp
// Both devices:
#define LORA_DISABLE_BAUD_SEARCH true   // Predictable behavior
#define LORA_FIXED_BAUD_RATE 9600       // Standard rate
#define LORA_INIT_TIMEOUT_MS 60000      // Patient but not excessive
```

### For Development/Testing

```cpp
// Both devices:
#define LORA_DISABLE_BAUD_SEARCH false  // Auto-detect
#define LORA_INIT_TIMEOUT_MS 60000      // Long enough for debugging
```

## Setting Module Baud Rate

To manually configure your Grove-Wio-E5 module to 9600 baud:

```cpp
// One-time configuration via serial terminal:
AT+UART=9600
```

Or use a USB-TTL adapter and terminal program (115200 default):

```
AT+UART=9600
```

The module will respond with `+OK` and restart at the new baud rate.

## Summary

- **Default behavior:** Auto-detect baud rate, wait 60 seconds
- **Recommended for production:** Fixed 9600 baud, 60-second timeout
- **Both devices must use compatible settings**
- **Patient initialization prevents false failures from slow M5Dial startup**
