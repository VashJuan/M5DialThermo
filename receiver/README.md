# Thermostat Receiver

This folder contains the receiving device/relay component of the M5Stack Dial
thermostat system.

> **📚 For complete documentation**, see the main project guides:
>
> - [USER_GUIDE.md](../doc/USER_GUIDE.md) - Operation and features
> - [HARDWARE_GUIDE.md](../doc/HARDWARE_GUIDE.md) - Pin connections and wiring
> - [NETWORK_GUIDE.md](../doc/NETWORK_GUIDE.md) - Communication setup and
>   troubleshooting
> - [DEVELOPER_GUIDE.md](../doc/DEVELOPER_GUIDE.md) - Build instructions and
>   code architecture

## Quick Reference

**Hardware**: XIAO ESP32S3 + Grove-Wio-E5 LoRa Module

**Key Pins**:

- D10 (GPIO10): Stove relay control
- D9 (GPIO9): Status LED
- D6/D7 (GPIO43/44): UART to LoRa module

**Build & Upload**:

```bash
cd receiver
pio run --target upload --upload-port COM6
cd ..
pio device monitor COM6
```

## LoRa Communication

Both the receiver and M5Dial transmitter use the **andresoliva/LoRa-E5 library**
for Grove-Wio-E5 module communication. This provides:

- **Ultra-low power:** 21µA sleep current (months of battery life)
- **Enhanced reliability:** Automatic retry and robust error handling
- **Signal monitoring:** RSSI, SNR, data rate reporting
- **Full features:** P2P, LoRaWAN, confirmed messages

**For complete details, see:**

- [DEVELOPER_GUIDE.md](../doc/DEVELOPER_GUIDE.md#lora-communication-library) -
  Library usage and code examples
- [HARDWARE_GUIDE.md](../doc/HARDWARE_GUIDE.md#grove-wio-e5-power-consumption) -
  Power consumption and battery life
- [NETWORK_GUIDE.md](../doc/NETWORK_GUIDE.md) - Communication setup and
  troubleshooting
