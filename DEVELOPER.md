# 🛠️ Developer Documentation

## 💻 Development Environment Setup

This project uses a VS Code workspace configuration with PlatformIO for optimal
development experience. The workspace includes multi-folder support for managing
the main thermostat, receiver, and shared components.

### 🚀 Quick Setup for New Developers

1. **📁 Clone/Download the project structure:**

   ```
   YourProjectFolder/
   ├── M5 Stack Dial/
   │   ├── thermo/               # Main thermostat (this folder)
   │   │   ├── __HouseThermo.code-workspace  # VS Code workspace config
   │   │   ├── platformio.ini    # PlatformIO project config
   │   │   └── src/              # Source code
   │   └── libraries/            # Additional libraries if needed
   └── __HouseThermo/           # Parent project folder (if applicable)
   ```

2. **🔧 Install Required Tools:**

   - **Visual Studio Code** - https://code.visualstudio.com/
   - **PlatformIO Extension** - Install from VS Code Extensions marketplace
   - **C/C++ Extension** - For IntelliSense and debugging support

3. **📂 Open the Workspace:**

   ```bash
   # Navigate to the thermo directory
   cd "path/to/your/thermo"

   # Open the workspace file (recommended)
   code __HouseThermo.code-workspace

   # OR open the folder directly
   code .
   ```

### 🛠️ VS Code Workspace Configuration

The project includes a `__HouseThermo.code-workspace` file that configures:

- **Multi-folder workspace** with main project, libraries, and examples
- **PlatformIO integration** with optimized settings:
  - Disables automatic PlatformIO Home startup
  - Activates only on PlatformIO projects
  - Prevents auto-opening of platformio.ini
- **Spell checking** with custom technical terms (lgfx, Orbitron)

### ⚙️ Recommended VS Code Extensions

For the best development experience, install these extensions:

1. **PlatformIO IDE** (platformio.platformio-ide) - ✅ Required
2. **C/C++** (ms-vscode.cpptools) - IntelliSense and debugging
3. **C/C++ Extension Pack** (ms-vscode.cpptools-extension-pack) - Complete C++
   support
4. **Code Spell Checker** (streetsidesoftware.code-spell-checker) - Document
   quality
5. **GitLens** (eamodio.gitlens) - Git integration and history
6. **Bracket Pair Colorizer** (CoenraadS.bracket-pair-colorizer-2) - Code
   readability

### 🎯 Development Workflow

1. **📂 Open Workspace:** Use `__HouseThermo.code-workspace` for multi-folder
   support
2. **🔧 Configure Secrets:** Copy `secrets_template.h` to `secrets.h` and add
   your WiFi credentials
3. **🔨 Build:** Use PlatformIO build tasks (Ctrl+Shift+P → "PlatformIO: Build")
4. **📤 Upload:** PlatformIO upload tasks or use `pio run --target upload`
5. **📊 Monitor:** Built-in serial monitor via PlatformIO

### 🌐 Setting Up on Different Machines

To recreate this development environment elsewhere:

1. **📋 Copy these essential files:**

   ```
   __HouseThermo.code-workspace    # VS Code workspace config
   platformio.ini                  # PlatformIO project config
   .gitignore                     # Git ignore patterns
   src/                           # Source code directory
   shared/                        # Shared protocol definitions
   ```

2. **📝 Workspace file structure explanation:**

   ```jsonc
   {
     "folders": [
       { "path": "../../__HouseThermo" },     # Parent project (if exists)
       { "path": ".." },                      # Current project root
       { "path": "../../libraries/M5Dial/examples/Basic" }  # Examples/references
     ],
     "settings": {
       "platformio-ide.autoOpenPlatformIOIniFile": false,
       "platformio-ide.activateOnlyOnPlatformIOProject": true,
       // ... other optimized settings
     }
   }
   ```

3. **🔧 Adapt paths for your structure:**

   - Modify the `"path"` entries in the workspace file to match your directory
     structure
   - The `".."` path should point to your main project directory
   - Additional paths are optional and can be removed if not applicable

4. **🚀 Platform-specific setup:**

   **Windows:**

   ```powershell
   # Clone and setup
   git clone <your-repo-url>
   cd "path\to\thermo"
   code __HouseThermo.code-workspace
   ```

   **macOS/Linux:**

   ```bash
   # Clone and setup
   git clone <your-repo-url>
   cd "path/to/thermo"
   code __HouseThermo.code-workspace
   ```

### 🔍 Troubleshooting Development Environment

- **❌ PlatformIO not working:** Ensure PlatformIO extension is installed and
  enabled
- **❌ IntelliSense errors:** Reload window (Ctrl+Shift+P → "Developer: Reload
  Window")
- **❌ Missing dependencies:** Run `pio lib install` to install missing
  libraries
- **❌ COM port issues:** Check device manager and ensure M5Dial is connected
- **❌ Workspace folders missing:** Adjust paths in
  `__HouseThermo.code-workspace`

### 💡 Development Tips

- **🔄 Use PlatformIO tasks:** Access via Command Palette (Ctrl+Shift+P)
- **🏗️ Build shortcuts:** Ctrl+Alt+B for build, Ctrl+Alt+U for upload
- **📱 Serial monitoring:** Built into PlatformIO, auto-detects baud rate
- **🎯 Multi-target support:** The platformio.ini supports multiple board
  configurations
- **⚡ Fast iteration:** Use `pio run -t upload && pio device monitor` for quick
  test cycles

## 🐠 PlatformIO Commands

- 📁 `pio run --target uploadfs` - Upload filesystem data (temps.csv, etc.) to
  device
- 🚀 `pio run --target upload` - Upload firmware to device
- 🛠️ `pio run` - Build project without uploading

**📝 Note**: The filesystem upload (`uploadfs`) must be done at least once
before first use, and again whenever you modify `temps.csv` or other data files.

## 📁 Code Structure

- 📋 `_thermo.cpp` - Main Arduino sketch with power management
- 🎛️ `encoder.cpp/.hpp` - Dial encoder handling
- 🌡️ `temp_sensor.cpp/.hpp` - Temperature sensor interface with caching
- 🔥 `stove.cpp/.hpp` - Stove control logic
- ⏰ `rtc.cpp/.hpp` - Real-time clock functionality
- 📱 `display.cpp/.hpp` - Display management with power-aware updates
- 📚 `doc/POWER_MANAGEMENT.md` - Detailed power saving documentation
- 📖 `doc/MCP9808_USAGE_EXAMPLE.md` - Temperature sensor usage guide

## 🔒 Security Configuration Setup

### 📶 WiFi Credentials Setup

To protect sensitive information like WiFi credentials, this project uses a
separate secrets file that is not committed to version control.

### 🚀 Initial Setup

1. **📋 Copy the template file:**

   ```bash
   cp secrets_template.h secrets.h
   ```

2. **✏️ Edit `secrets.h` with your actual credentials:**

   ```cpp
   #define DEFAULT_WIFI_SSID "YourActualWiFiName"
   #define DEFAULT_WIFI_PASSWORD "YourActualWiFiPassword"
   ```

3. **✅ Verify `secrets.h` is in `.gitignore`:** The file `secrets.h` should
   already be listed in `.gitignore` to prevent accidental commits.

### ⚠️ Important Notes

- ⚠️ **Never commit `secrets.h` to version control**
- ✅ Always use `secrets_template.h` as a reference for the required structure
- 🔒 Keep your actual credentials in `secrets.h` only
- 📝 Update `secrets_template.h` if you add new secret configuration options

### File Structure

- `secrets_template.h` - Template file (committed to Git)
- `secrets.h` - Your actual credentials (ignored by Git)
- `.gitignore` - Contains entry for `secrets.h`

### Adding New Secrets

If you need to add new sensitive configuration (API keys, MQTT credentials,
etc.):

1. Add the `#define` to both `secrets_template.h` and `secrets.h`
2. Use placeholder values in `secrets_template.h`
3. Use real values in `secrets.h`

### Troubleshooting

If you get compilation errors about missing `secrets.h`:

1. Make sure you copied `secrets_template.h` to `secrets.h`
2. Verify the file exists in the same directory as `rtc.cpp`
3. Check that your WiFi credentials are properly defined in `secrets.h`

## 💻 Development Environment

- **Platform**: Arduino M5Stack Board Manager v2.0.7
- **IDE**: PlatformIO within Visual Studio Code, or Arduino IDE

## 📦 Current Library Dependencies

- M5Unified @ 0.1.17
- M5GFX @ 0.1.17
- Adafruit MCP9808 Library @ 2.0.2
- ArduinoJson @ 7.4.2
- HTTPClient @ 2.0.0
- SPIFFS @ 2.0.0
- FS @ 2.0.0
- WiFi @ 2.0.0
- Wire @ 2.0.0

## 📝 Technical Notes

- ⚠️ Initial implementation assumes I2C interface for temperature sensor and
  Wio-E5 module
- 🔧 These modules do not actually support I2C - interface may need revision
- 🌡️ Temperature sensor uses analog interface
- 📡 Wio-E5 uses UART/AT commands for LoRaWAN communication

### 📝 Note

The warning about flash size (16MB vs 8MB available) is just a configuration
mismatch but doesn't prevent operation since the actual firmware (889KB) fits
comfortably in the available 8MB flash.

## 📚 Additional Technical Documentation

- **[Power Management Guide](doc/POWER_MANAGEMENT.md)** - Detailed information
  about power saving features, battery optimization, and energy consumption
- **[MCP9808 Sensor Usage](doc/MCP9808_USAGE_EXAMPLE.md)** - Complete guide for
  the precision temperature sensor including caching features
- **[Timezone Configuration](README_TIMEZONE.md)** - Geographic timezone
  detection and fallback systems
- **[NTP Troubleshooting](README_NTP_TROUBLESHOOTING.md)** - Network time
  synchronization debugging
- **[Build Instructions](README_BUILD.md)** - Compilation and upload procedures

## 👨‍💻 Contributing

When contributing to this project:

1. Follow the existing code style and commenting patterns
2. Update relevant documentation when making changes
3. Test on actual hardware when possible
4. Keep commits focused and well-documented
5. Ensure secrets are properly handled (use templates)
