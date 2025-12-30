/**
 * @file lora_transmitter.hpp
 * @brief LoRaWAN transmitter class for Grove-Wio-E5 communication
 * @version 1.0.0
 * @date 2025-12-30
 */

#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include "../shared/protocol_common.hpp"

/**
 * @class LoRaTransmitter
 * @brief Handles LoRaWAN transmitter functionality using Grove-Wio-E5 module
 *
 * This class manages the LoRaWAN transmitter functionality using the Grove-Wio-E5
 * module connected via UART. It mirrors the LoRaReceiver class but focuses on
 * transmitting commands and receiving acknowledgments.
 */
class LoRaTransmitter
{
private:
    HardwareSerial *loraSerial;
    int rxPin;
    int txPin;
    bool isInitialized;
    LoRaWANConfig config;

    // Statistics and performance tracking
    unsigned long lastTransmissionTime;
    unsigned long lastAckTime;
    int successfulTransmissions;
    int failedTransmissions;
    int totalRetries;
    String lastError;

    // AT command handling with enhanced features from Grove-Wio-E5 examples
    bool sendATCommand(const String &command, const String &expectedResponse = "OK", int timeout = 5000);
    String readResponse(int timeout = 5000);
    void clearSerialBuffer();

    // Enhanced response parsing with timing measurements (inspired by Grove-Wio-E5 time measures example)
    bool sendATCommandWithTiming(const String &command, const String &expectedResponse, int timeout, unsigned long &commandTime);
    bool waitForTransmissionComplete(unsigned long &txTime, unsigned long &ackTime);

    // LoRaWAN configuration
    bool configureLoRaWAN();
    bool joinNetwork();

    // Message handling
    String createHexMessage(const String &command, uint8_t port = LORAWAN_PORT_CONTROL);
    bool sendMessage(const String &hexMessage, bool confirmed = true);

public:
    /**
     * @brief Constructor
     */
    LoRaTransmitter();

    /**
     * @brief Destructor
     */
    ~LoRaTransmitter();

    /**
     * @brief Initialize the LoRa transmitter
     * @param rxPin UART RX pin (connected to Grove-Wio-E5 TX)
     * @param txPin UART TX pin (connected to Grove-Wio-E5 RX)
     * @param loraConfig LoRaWAN configuration (optional, uses default if not provided)
     * @return true if initialization successful
     */
    bool setup(int rxPin, int txPin, const LoRaWANConfig &loraConfig = LoRaWANConfig());

    /**
     * @brief Send a command via LoRaWAN
     * @param command Command string to send (e.g., "STOVE_ON", "STOVE_OFF")
     * @param port LoRaWAN port number (default: LORAWAN_PORT_CONTROL)
     * @param confirmed Whether to request confirmation (default: true)
     * @param maxRetries Maximum number of retry attempts (default: 3)
     * @return Response string from receiver, empty string if failed
     */
    String sendCommand(const String &command, uint8_t port = LORAWAN_PORT_CONTROL,
                       bool confirmed = true, int maxRetries = 3);

    /**
     * @brief Send a ping message to test connectivity
     * @return true if ping successful (pong received)
     */
    bool ping();

    /**
     * @brief Send a status request
     * @return Status response from receiver, empty string if failed
     */
    String requestStatus();

    /**
     * @brief Get signal quality information
     * @return Signal quality string (RSSI, SNR, etc.)
     */
    String getSignalQuality();

    /**
     * @brief Check if module is ready for communication
     * @return true if ready
     */
    bool isReady();

    /**
     * @brief Reset the LoRa module
     * @return true if reset successful
     */
    bool reset();

    /**
     * @brief Enter low power mode to save energy
     * @return true if successful
     */
    bool enterLowPowerMode();

    /**
     * @brief Wake up the module from low power mode
     * @return true if successful
     */
    bool wakeUp();

    /**
     * @brief Set automatic low power mode (module sleeps/wakes automatically)
     * @param enable true to enable auto mode, false to disable
     * @return true if successful
     */
    bool setAutoLowPowerMode(bool enable);

    /**
     * @brief Get transmission statistics
     * @return Statistics string with success/failure counts and timing info
     */
    String getStatistics();

    /**
     * @brief Get last error message
     * @return Last error string
     */
    String getLastError();

    /**
     * @brief Clear transmission statistics
     */
    void clearStatistics();

    /**
     * @brief Set LoRaWAN configuration
     * @param loraConfig New configuration
     * @return true if configuration applied successfully
     */
    bool setConfiguration(const LoRaWANConfig &loraConfig);

    /**
     * @brief Get current LoRaWAN configuration
     * @return Current configuration
     */
    LoRaWANConfig getConfiguration();

    /**
     * @brief Check if currently joined to LoRaWAN network
     * @return true if joined
     */
    bool isJoined();

    /**
     * @brief Force rejoin to LoRaWAN network
     * @return true if rejoin successful
     */
    bool rejoin();

    /**
     * @brief Send raw hex data (for advanced users)
     * @param hexData Hex string to send
     * @param port LoRaWAN port
     * @param confirmed Whether to request confirmation
     * @return true if sent successfully
     */
    bool sendRawHex(const String &hexData, uint8_t port = LORAWAN_PORT_CONTROL, bool confirmed = true);

    /**
     * @brief Get device information (DevEUI, AppEUI, etc.)
     * @return Device info string
     */
    String getDeviceInfo();
};