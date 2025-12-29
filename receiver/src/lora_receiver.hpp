/**
 * @file lora_receiver.hpp
 * @brief LoRaWAN receiver class for Grove-Wio-E5 communication
 * @version 1.0.0
 * @date 2025-12-29
 */

#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

/**
 * @class LoRaReceiver
 * @brief Handles LoRaWAN communication with Grove-Wio-E5 module
 *
 * This class manages the LoRaWAN receiver functionality using the Grove-Wio-E5
 * module connected via UART to the XIAO ESP32S3.
 */
class LoRaReceiver
{
private:
    HardwareSerial *loraSerial;
    int rxPin;
    int txPin;
    bool isInitialized;

    // AT command handling
    bool sendATCommand(const String &command, const String &expectedResponse = "OK", int timeout = 5000);
    String readResponse(int timeout = 5000);
    void clearSerialBuffer();

    // LoRaWAN configuration
    bool configureLoRaWAN();
    bool joinNetwork();

public:
    /**
     * @brief Constructor
     */
    LoRaReceiver();

    /**
     * @brief Destructor
     */
    ~LoRaReceiver();

    /**
     * @brief Initialize the LoRa receiver
     * @param rxPin UART RX pin (connected to Grove-Wio-E5 TX)
     * @param txPin UART TX pin (connected to Grove-Wio-E5 RX)
     * @return true if initialization successful
     */
    bool setup(int rxPin, int txPin);

    /**
     * @brief Check for incoming LoRa commands
     * @return Command string if received, empty string if none
     */
    String checkForCommand();

    /**
     * @brief Send response back to transmitter
     * @param response Response string to send
     * @return true if sent successfully
     */
    bool sendResponse(const String &response);

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
};