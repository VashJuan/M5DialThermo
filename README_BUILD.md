# M5Dial Thermostat Project

## Build and Upload Instructions

### Using PlatformIO (Recommended)

1. **Install PlatformIO:**

   ```bash
   # If you have Python/pip installed:
   pip install platformio

   # Or install PlatformIO IDE in VS Code
   ```

2. **Build the project:**

   ```bash
   cd "d:\Projects\Arduino\M5 Stack Dial\thermo"
   pio run
   ```

3. **Upload to M5Dial (COM4):**

   ```bash
   pio run --target upload
   ```

4. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

### Using Arduino IDE

1. **Board Settings:**

   - Board: "M5Stack-CoreS3" or "ESP32S3 Dev Module"
   - Port: COM4
   - Upload Speed: 921600
   - Flash Mode: QIO
   - Flash Size: 8MB
   - Partition Scheme: Default 8MB

2. **Required Libraries:**

   - M5Unified (v0.1.13+)
   - M5GFX (v0.1.15+)
   - Adafruit MCP9808 Library (v2.0.1+)

3. **Upload Process:**
   - Open `thermo.ino` in Arduino IDE
   - Select correct board and port
   - Click Upload button

## Troubleshooting

- **Upload fails:** Try holding the reset button during upload
- **COM port not found:** Check device manager and cable connection
- **Compilation errors:** Ensure all .hpp/.cpp files are in same directory
- **Missing libraries:** Install through Library Manager in Arduino IDE

## Project Structure

```
thermo/
├── thermo.ino          # Main sketch
├── display.hpp/.cpp    # Display abstraction
├── stove.hpp/.cpp     # Stove control logic
├── rtc.hpp/.cpp       # Real-time clock
├── temp_sensor.hpp/.cpp # Temperature sensor
├── encoder.hpp/.cpp   # Rotary encoder
├── secrets.h          # WiFi credentials (create this)
└── platformio.ini     # PlatformIO configuration
```
