/**
 * @file protocol_common.hpp
 * @brief Shared communication protocol definitions for LoRaWAN thermostat system
 * @version 1.0.0
 * @date 2025-12-29
 */

#pragma once

#include <Arduino.h>

// LoRaWAN Communication Protocol for M5Stack Dial Thermostat System

// Command definitions (transmitted as ASCII strings)
#define CMD_STOVE_ON "STOVE_ON"
#define CMD_STOVE_OFF "STOVE_OFF"
#define CMD_STATUS_REQUEST "STATUS_REQUEST"
#define CMD_PING "PING"

// Response definitions
#define RESP_ACK "ACK"
#define RESP_NACK "NACK"
#define RESP_STOVE_ON "STOVE_ON"
#define RESP_STOVE_OFF "STOVE_OFF"
#define RESP_PONG "PONG"
#define RESP_ERROR "ERROR"
#define RESP_TIMEOUT "SAFETY_TIMEOUT"
#define RESP_UNKNOWN "ERROR_UNKNOWN_COMMAND"

// LoRaWAN Configuration Constants
// Note: These should match between transmitter and receiver

// Region settings
#define LORAWAN_REGION_US915 "US915"
#define LORAWAN_REGION_EU868 "EU868"
#define LORAWAN_DEFAULT_REGION LORAWAN_REGION_US915 // Change based on location

// Data rates
#define LORAWAN_DR_SLOW 0   // Longest range, slowest speed
#define LORAWAN_DR_MEDIUM 3 // Balanced
#define LORAWAN_DR_FAST 5   // Shortest range, fastest speed
#define LORAWAN_DEFAULT_DR LORAWAN_DR_MEDIUM

// Timing constants
#define LORAWAN_JOIN_TIMEOUT 30000 // 30 seconds to join network
#define LORAWAN_TX_TIMEOUT 10000   // 10 seconds for transmission
#define LORAWAN_RX_TIMEOUT 5000    // 5 seconds to wait for response

// Port numbers for different message types
#define LORAWAN_PORT_CONTROL 1 // Stove control commands
#define LORAWAN_PORT_STATUS 2  // Status requests/responses
#define LORAWAN_PORT_PING 3    // Ping/pong for connectivity testing

// Message structure
struct LoRaWANMessage
{
    uint8_t port;       // LoRaWAN port
    String payload;     // ASCII command/response
    uint32_t timestamp; // Message timestamp (optional)
    uint8_t retryCount; // Retry attempt number
};

// Helper functions for message encoding/decoding
class ProtocolHelper
{
public:
    /**
     * @brief Convert ASCII string to hex for LoRaWAN transmission
     * @param ascii ASCII string to convert
     * @return Hex string representation
     */
    static String asciiToHex(const String &ascii)
    {
        String hex = "";
        for (int i = 0; i < ascii.length(); i++)
        {
            char hexByte[3];
            sprintf(hexByte, "%02X", (int)ascii[i]);
            hex += hexByte;
        }
        return hex;
    }

    /**
     * @brief Convert hex string to ASCII
     * @param hex Hex string to convert
     * @return ASCII string representation
     */
    static String hexToAscii(const String &hex)
    {
        String ascii = "";
        for (int i = 0; i < hex.length(); i += 2)
        {
            if (i + 1 < hex.length())
            {
                String hexByte = hex.substring(i, i + 2);
                char asciiChar = (char)strtol(hexByte.c_str(), NULL, 16);
                ascii += asciiChar;
            }
        }
        return ascii;
    }

    /**
     * @brief Create a formatted LoRaWAN command
     * @param command Command string
     * @param port LoRaWAN port number
     * @return Formatted hex string ready for transmission
     */
    static String createMessage(const String &command, uint8_t port = LORAWAN_PORT_CONTROL)
    {
        // Simple format: just convert command to hex
        // Could be extended to include headers, checksums, etc.
        return asciiToHex(command);
    }

    /**
     * @brief Parse received LoRaWAN message
     * @param hexData Received hex data
     * @param port Received port number
     * @return Parsed command string
     */
    static String parseMessage(const String &hexData, uint8_t port = LORAWAN_PORT_CONTROL)
    {
        // Simple format: just convert hex to ASCII
        // Could be extended to handle headers, validate checksums, etc.
        return hexToAscii(hexData);
    }

    /**
     * @brief Validate command string
     * @param command Command to validate
     * @return true if command is valid
     */
    static bool isValidCommand(const String &command)
    {
        return (command == CMD_STOVE_ON ||
                command == CMD_STOVE_OFF ||
                command == CMD_STATUS_REQUEST ||
                command == CMD_PING);
    }

    /**
     * @brief Validate response string
     * @param response Response to validate
     * @return true if response is valid
     */
    static bool isValidResponse(const String &response)
    {
        return (response == RESP_ACK ||
                response == RESP_NACK ||
                response == RESP_STOVE_ON ||
                response == RESP_STOVE_OFF ||
                response == RESP_PONG ||
                response == RESP_ERROR ||
                response == RESP_TIMEOUT ||
                response == RESP_UNKNOWN);
    }
};

// Default LoRaWAN network configuration
// NOTE: These are example values - replace with your actual network configuration
struct LoRaWANConfig
{
    String appEUI = "0000000000000000";                 // Application EUI (8 bytes hex)
    String appKey = "00000000000000000000000000000000"; // Application Key (16 bytes hex)
    String region = LORAWAN_DEFAULT_REGION;             // Frequency region
    uint8_t dataRate = LORAWAN_DEFAULT_DR;              // Data rate
    bool adaptiveDataRate = true;                       // Enable ADR
    uint8_t transmitPower = 14;                         // TX power (dBm)

    // Device-specific settings
    bool otaa = true;           // Over-The-Air Activation
    uint8_t confirmUplinks = 1; // Confirmed uplinks (0=unconfirmed, 1=confirmed)
    uint8_t maxRetries = 3;     // Maximum retries for confirmed messages
};