# Security Configuration Setup

## WiFi Credentials Setup

To protect sensitive information like WiFi credentials, this project uses a
separate secrets file that is not committed to version control.

### Initial Setup

1. **Copy the template file:**

   ```bash
   cp secrets_template.h secrets.h
   ```

2. **Edit `secrets.h` with your actual credentials:**

   ```cpp
   #define DEFAULT_WIFI_SSID "YourActualWiFiName"
   #define DEFAULT_WIFI_PASSWORD "YourActualWiFiPassword"
   ```

3. **Verify `secrets.h` is in `.gitignore`:** The file `secrets.h` should
   already be listed in `.gitignore` to prevent accidental commits.

### Important Notes

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
