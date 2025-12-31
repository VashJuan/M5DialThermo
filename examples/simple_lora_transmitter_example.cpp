/**
 * @file simple_lora_transmitter_example.cpp
 * @brief Simple example demonstrating LoRa transmitter functionality
 * @version 1.0.0
 * @date 2025-12-30
 *
 * This is a simplified example that shows how to use the LoRaTransmitter class
 * to send commands to a remote LoRa receiver.
 */

#include <Arduino.h>
#include "lora_transmitter.hpp"

// LoRa transmitter instance
LoRaTransmitter transmitter;

// Pin configuration for Grove-Wio-E5 connection
const int LORA_RX_PIN = 44;  // Connect to Grove-Wio-E5 TX
const int LORA_TX_PIN = 43;  // Connect to Grove-Wio-E5 RX

// LoRaWAN configuration - Update with your actual network settings
LoRaWANConfig config = {
    .appEUI = "70B3D57ED0000000",                       // Replace with your AppEUI  
    .appKey = "A1B2C3D4E5F6708192A3B4C5D6E7F801",       // Replace with your AppKey
    .region = LORAWAN_REGION_US915,                     // Change to EU868 if in Europe
    .dataRate = LORAWAN_DR_MEDIUM,
    .adaptiveDataRate = true,
    .transmitPower = 14,
    .otaa = true,
    .confirmUplinks = 1,
    .maxRetries = 3
};

void setup()
{
    Serial.begin(115200);
    delay(2000); // Allow serial monitor to connect
    
    Serial.println("LoRa Transmitter Example");
    Serial.println("========================");
    
    // Initialize the LoRa transmitter
    Serial.println("Initializing LoRa transmitter...");
    
    if (transmitter.setup(LORA_RX_PIN, LORA_TX_PIN, config)) {
        Serial.println("✓ LoRa transmitter initialized successfully");
        
        // Display device information
        Serial.println("\nDevice Information:");
        Serial.println(transmitter.getDeviceInfo());
        
        // Test connectivity
        Serial.println("Testing connectivity...");
        if (transmitter.ping()) {
            Serial.println("✓ Ping successful - receiver is responding");
        } else {
            Serial.println("✗ Ping failed - no response from receiver");
        }
        
    } else {
        Serial.println("✗ LoRa transmitter initialization failed");
        Serial.println("Last error: " + transmitter.getLastError());
        
        // Don't proceed with the example
        while (true) {
            delay(1000);
        }
    }
    
    Serial.println("\nSetup complete. Commands will be sent every 30 seconds.");
    Serial.println("Available commands: STOVE_ON, STOVE_OFF, STATUS_REQUEST, PING\n");
}

void loop()
{
    static unsigned long lastCommandTime = 0;
    static int commandIndex = 0;
    
    // Send a command every 30 seconds
    if (millis() - lastCommandTime >= 30000) {
        
        // Array of commands to cycle through
        String commands[] = {
            CMD_PING,
            CMD_STATUS_REQUEST, 
            CMD_STOVE_ON,
            CMD_STATUS_REQUEST,
            CMD_STOVE_OFF,
            CMD_STATUS_REQUEST
        };
        
        int numCommands = sizeof(commands) / sizeof(commands[0]);
        String command = commands[commandIndex % numCommands];
        
        Serial.println("==========================================");
        Serial.printf("Sending command #%d: %s\n", commandIndex + 1, command.c_str());
        Serial.println("==========================================");
        
        // Record start time
        unsigned long startTime = millis();
        
        // Send the command
        String response = transmitter.sendCommand(command, LORAWAN_PORT_CONTROL, true, 2);
        
        // Calculate transmission time
        unsigned long transmissionTime = millis() - startTime;
        
        // Display results
        if (response.length() > 0) {
            Serial.printf("✓ Response received: %s (took %lu ms)\n", 
                         response.c_str(), transmissionTime);
                         
            // Interpret the response
            if (response == RESP_PONG) {
                Serial.println("  → Receiver is alive and responding");
            } else if (response == RESP_STOVE_ON) {
                Serial.println("  → Stove is ON");
            } else if (response == RESP_STOVE_OFF) {
                Serial.println("  → Stove is OFF");
            } else if (response == RESP_ACK) {
                Serial.println("  → Command acknowledged");
            } else if (response == RESP_NACK) {
                Serial.println("  → Command rejected");
            } else if (response == RESP_ERROR) {
                Serial.println("  → Error occurred (possibly safety limit)");
            } else {
                Serial.printf("  → Unknown response: %s\n", response.c_str());
            }
            
        } else {
            Serial.printf("✗ No response received (took %lu ms)\n", transmissionTime);
            Serial.println("  → Check receiver status or network connectivity");
        }
        
        // Display signal quality
        String signalQuality = transmitter.getSignalQuality();
        if (signalQuality.length() > 0) {
            Serial.printf("📡 Signal: %s\n", signalQuality.c_str());
        }
        
        // Display statistics
        Serial.println("\nTransmission Statistics:");
        Serial.println(transmitter.getStatistics());
        
        lastCommandTime = millis();
        commandIndex++;
        
        Serial.println("Waiting 30 seconds before next command...\n");
    }
    
    // Check if module is still ready
    static unsigned long lastReadyCheck = 0;
    if (millis() - lastReadyCheck >= 60000) { // Check every minute
        if (!transmitter.isReady()) {
            Serial.println("⚠ Warning: LoRa module not responding");
            
            // Attempt to reset and reconnect
            Serial.println("Attempting to reset module...");
            if (transmitter.reset()) {
                Serial.println("✓ Module reset successful");
            } else {
                Serial.println("✗ Module reset failed");
            }
        }
        lastReadyCheck = millis();
    }
    
    // Small delay to prevent overwhelming the processor
    delay(100);
}

// Optional: Add interrupt handlers for manual command sending
void sendManualCommand(const String& command)
{
    Serial.printf("\n[MANUAL] Sending command: %s\n", command.c_str());
    
    String response = transmitter.sendCommand(command);
    
    if (response.length() > 0) {
        Serial.printf("[MANUAL] Response: %s\n", response.c_str());
    } else {
        Serial.println("[MANUAL] No response received");
    }
    
    Serial.println();
}