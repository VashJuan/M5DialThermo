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
#define RESP_STOVE_ON_ACK "STOVE_ON_ACK"
#define RESP_STOVE_OFF_ACK "STOVE_OFF_ACK"
#define RESP_STATUS "STATUS_OK"
#define RESP_PONG "PONG"
#define RESP_ERROR "ERROR"
#define RESP_TIMEOUT "SAFETY_TIMEOUT"
#define RESP_UNKNOWN "ERROR_UNKNOWN_COMMAND"

// P2P Configuration Constants
// Based on Grove Wio E5 P2P example
#define P2P_FREQUENCY 915           // Frequency in MHz - US (915 MHz) default, change to 866 for EU
#define P2P_SPREADING_FACTOR "SF12" // Spreading factor (SF7-SF12)
#define P2P_BANDWIDTH "125"         // Bandwidth in kHz
#define P2P_CODING_RATE "12"        // Coding rate 4/5, 4/6, 4/7, 4/8 -> 5,6,7,8
#define P2P_PREAMBLE_LENGTH "15"    // Preamble length
#define P2P_TX_POWER "14"           // TX power in dBm
#define P2P_CRC "ON"                // CRC enable/disable
#define P2P_IQ_INVERSION "OFF"      // IQ inversion
#define P2P_SYNC_WORD "OFF"         // Sync word

// P2P timeout constants
#define P2P_TX_TIMEOUT 3000  // 3 second transmit timeout
#define P2P_RX_TIMEOUT 13000 // 13 second receive timeout (allows 11s RX window + overhead)
#define P2P_POWER 14         // P2P power in dBm (for compatibility)

// Communication mode selection
enum class LoRaCommunicationMode
{
    P2P = 0,    // Point-to-point communication (default)
    LoRaWAN = 1 // LoRaWAN network communication (fallback)
};

// P2P command prefix for protocol identification
#define P2P_MSG_PREFIX "THERMO" // 6 chars prefix to identify our messages

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
     * @brief Validate if a response is recognized
     * @param response Response string to validate
     * @return true if response is valid
     */
    static bool isValidResponse(const String &response)
    {
        return (response == RESP_STOVE_ON_ACK || response == RESP_STOVE_OFF_ACK ||
                response == RESP_PONG || response == RESP_STATUS ||
                response.startsWith("STATUS:") || response == "SENT");
    }
    /**
     * @brief Create a P2P message with prefix for identification
     * @param command Command string
     * @return Formatted P2P message
     */
    static String createP2PMessage(const String &command)
    {
        // Format: PREFIX + command (e.g., "THERMOSTOVE_ON")
        return String(P2P_MSG_PREFIX) + command;
    }

    /**
     * @brief Parse received P2P message and extract command
     * @param message Received P2P message
     * @return Extracted command string, empty if invalid prefix
     */
    static String parseP2PMessage(const String &message)
    {
        String prefix = String(P2P_MSG_PREFIX);
        if (message.startsWith(prefix))
        {
            return message.substring(prefix.length());
        }
        return ""; // Invalid prefix
    }

    /**
     * @brief Check if message is a valid P2P thermostat message
     * @param message Message to check
     * @return true if valid P2P message
     */
    static bool isValidP2PMessage(const String &message)
    {
        return message.startsWith(P2P_MSG_PREFIX);
    }
};

// Default LoRaWAN network configuration
// NOTE: These are example values - replace with your actual network configuration
struct LoRaWANConfig
{
    // Communication mode selection
    LoRaCommunicationMode mode = LoRaCommunicationMode::P2P; // Default to P2P

    // P2P configuration (used when mode = LoRaCommunicationMode::P2P)
    uint16_t p2pFrequency = P2P_FREQUENCY;
    String p2pSpreadingFactor = P2P_SPREADING_FACTOR;
    String p2pBandwidth = P2P_BANDWIDTH;
    String p2pCodingRate = P2P_CODING_RATE;
    String p2pPreambleLength = P2P_PREAMBLE_LENGTH;
    String p2pTxPower = P2P_TX_POWER;
    String p2pCrc = P2P_CRC;
    String p2pIqInversion = P2P_IQ_INVERSION;
    String p2pSyncWord = P2P_SYNC_WORD;

    // LoRaWAN configuration (used when mode = LoRaCommunicationMode::LoRaWAN)
    // Credentials should be defined in secrets.h (LORAWAN_APP_EUI and LORAWAN_APP_KEY)
#ifdef LORAWAN_APP_EUI
    String appEUI = LORAWAN_APP_EUI; // Application EUI from secrets.h
#else
    String appEUI = "0000000000000000"; // Fallback placeholder (8 bytes hex)
#endif
#ifdef LORAWAN_APP_KEY
    String appKey = LORAWAN_APP_KEY; // Application Key from secrets.h
#else
    String appKey = "00000000000000000000000000000000"; // Fallback placeholder (16 bytes hex)
#endif
    String region = LORAWAN_DEFAULT_REGION; // Frequency region
    uint8_t dataRate = LORAWAN_DEFAULT_DR;  // Data rate
    bool adaptiveDataRate = true;           // Enable ADR
    uint8_t transmitPower = 14;             // TX power (dBm)

    // Device-specific settings
    bool otaa = true;           // Over-The-Air Activation
    uint8_t confirmUplinks = 1; // Confirmed uplinks (0=unconfirmed, 1=confirmed)
    uint8_t maxRetries = 3;     // Maximum retries for confirmed messages
};