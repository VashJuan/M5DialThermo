# ✅ Checklist: Enable Fixed 9600 Baud & Patient Connection

## Quick Enable Steps

### [ ] Step 1: Update Receiver

Open file: `receiver/src/lora_receiver.hpp`

Find line 15 (around line 15):

```cpp
#define LORA_DISABLE_BAUD_SEARCH false
```

Change to:

```cpp
#define LORA_DISABLE_BAUD_SEARCH true
```

### [ ] Step 2: Update Transmitter

Open file: `src/lora_transmitter.hpp`

Find line 15 (around line 15):

```cpp
#define LORA_TX_DISABLE_BAUD_SEARCH false
```

Change to:

```cpp
#define LORA_TX_DISABLE_BAUD_SEARCH true
```

### [ ] Step 3: Verify Settings (Optional)

Both files should also have these lines (usually don't need changing):

```cpp
#define LORA_FIXED_BAUD_RATE 9600       // Baud rate to use
#define LORA_INIT_TIMEOUT_MS 60000      // 60 seconds timeout
```

### [ ] Step 4: Build Receiver

- Open receiver project in PlatformIO
- Build the project
- Verify no compilation errors

### [ ] Step 5: Build Transmitter

- Open main project in PlatformIO
- Build the project
- Verify no compilation errors

### [ ] Step 6: Upload Receiver

- Connect XIAO ESP32S3 (receiver)
- Upload firmware
- Open Serial Monitor (115200 baud)

### [ ] Step 7: Upload Transmitter

- Connect M5Dial (transmitter)
- Upload firmware
- Open Serial Monitor (115200 baud)

### [ ] Step 8: Test Connection

Expected receiver output:

```
Using fixed baud rate: 9600 (baud search disabled)
Connection attempt 1 (elapsed: 3000 ms)...
  No response - waiting for M5Dial...
Connection attempt 2 (elapsed: 5200 ms)...
SUCCESS! Module responding at 9600 baud
```

Expected transmitter output:

```
Using fixed baud rate: 9600 (baud search disabled)
Connection attempt 1 (elapsed: 2000 ms)...
SUCCESS! Module responding at 9600 baud
```

### [ ] Step 9: Verify Operation

- [ ] Receiver successfully connects to LoRa module
- [ ] Transmitter successfully connects to LoRa module
- [ ] Connection happens within 60 seconds
- [ ] Both show "SUCCESS!" message
- [ ] System proceeds to normal operation

## If Connection Fails

### [ ] Troubleshooting Step 1: Check Physical Connections

- [ ] RX/TX not swapped
- [ ] 3.3V power stable
- [ ] Antenna attached to Grove-Wio-E5

### [ ] Troubleshooting Step 2: Increase Timeout

Both header files, change:

```cpp
#define LORA_INIT_TIMEOUT_MS 90000  // Try 90 seconds
```

### [ ] Troubleshooting Step 3: Try Auto-Detect Mode

Both header files, change back:

```cpp
#define LORA_DISABLE_BAUD_SEARCH false
#define LORA_TX_DISABLE_BAUD_SEARCH false
```

This will try multiple baud rates automatically.

### [ ] Troubleshooting Step 4: Check Module Baud Rate

Your Grove-Wio-E5 modules must be configured for 9600 baud. To set module baud
rate, connect via USB-TTL and send:

```
AT+UART=9600
```

## Success Indicators

✅ **You're successful when you see:**

- "SUCCESS! Module responding at 9600 baud" on both devices
- Connection within 60 seconds
- No timeout errors
- System proceeds to P2P or LoRaWAN configuration

## Optional: Adjust Settings

### For Faster Connection (if modules are fast):

```cpp
#define LORA_INIT_TIMEOUT_MS 30000  // 30 seconds
```

### For Slower M5Dial:

```cpp
#define LORA_INIT_TIMEOUT_MS 120000  // 2 minutes
```

### For Different Baud Rate:

```cpp
#define LORA_FIXED_BAUD_RATE 19200  // Or 115200
```

**Note:** Both devices must use the same baud rate!

## Documentation References

- 📚 Full details: `README_BAUD_CONFIGURATION.md`
- 🚀 Quick start: `QUICK_START_FIXED_BAUD.md`
- 📋 Summary: `SUMMARY_OF_CHANGES.md`

## Support

If issues persist after following this checklist:

1. Check serial monitor output for specific error messages
2. Review `README_BAUD_CONFIGURATION.md` troubleshooting section
3. Try auto-detect mode to isolate baud rate issues
4. Verify Grove-Wio-E5 modules are functioning correctly
