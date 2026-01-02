# Quick Start: Fixed 9600 Baud Configuration

## To enable fixed 9600 baud rate and skip baud searching:

### Step 1: Update Receiver

Edit `receiver/src/lora_receiver.hpp` - change line 9:

```cpp
#define LORA_DISABLE_BAUD_SEARCH true  // Changed from false to true
```

### Step 2: Update Transmitter

Edit `src/lora_transmitter.hpp` - change line 9:

```cpp
#define LORA_TX_DISABLE_BAUD_SEARCH true  // Changed from false to true
```

### Step 3: Rebuild and Upload

Rebuild both the receiver and transmitter projects and upload.

## What This Does:

✅ **Receiver** will:

- Use 9600 baud immediately (no searching)
- Wait up to 60 seconds for M5Dial to come online
- Keep trying patiently with 2-second intervals
- Display elapsed time on each attempt

✅ **Transmitter (M5Dial)** will:

- Use 9600 baud immediately (no searching)
- Wait patiently for connection (up to 60 seconds)
- Take 2-second pauses between connection attempts
- Not rush or timeout quickly

## Expected Serial Output:

### Receiver:

```
Setting up LoRa receiver on pins RX:44, TX:43
Waiting for Grove-Wio-E5 and M5Dial to power up and stabilize...
Initialization timeout: 60 seconds
Using fixed baud rate: 9600 (baud search disabled)
Connection attempt 1 (elapsed: 3000 ms)...
  No response - waiting for M5Dial...
Connection attempt 2 (elapsed: 5200 ms)...
  No response - waiting for M5Dial...
...
Connection attempt 8 (elapsed: 19400 ms)...
SUCCESS! Module responding at 9600 baud
Grove-Wio-E5 communication established
```

### Transmitter:

```
Setting up LoRa transmitter...
Setting up LoRa transmitter on pins RX:1, TX:2
Initializing LoRa module - patient connection mode enabled
Initialization timeout: 60 seconds
Using fixed baud rate: 9600 (baud search disabled)
Connection attempt 1 (elapsed: 2000 ms)...
SUCCESS! Module responding at 9600 baud
Grove-Wio-E5 communication established
```

## To Adjust Timeout:

If 60 seconds isn't enough, increase the timeout (e.g., to 90 seconds):

**Receiver** (`receiver/src/lora_receiver.hpp`):

```cpp
#define LORA_INIT_TIMEOUT_MS 90000  // 90 seconds
```

**Transmitter** (`src/lora_transmitter.hpp`):

```cpp
#define LORA_TX_INIT_TIMEOUT_MS 90000  // 90 seconds
```

## To Change Baud Rate:

If you want to use a different baud rate (e.g., 19200):

**Both files:**

```cpp
#define LORA_FIXED_BAUD_RATE 19200
```

**Note:** Your Grove-Wio-E5 modules must be configured for the same baud rate!
