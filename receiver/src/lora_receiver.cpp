/**
 * @file lora_receiver.cpp
 * @brief LoRaWAN receiver implementation for Grove-Wio-E5
 * @version 1.0.0
 * @date 2025-12-29
 */

#include "lora_receiver.hpp"
#include "secrets.h"

LoRaReceiver::LoRaReceiver() : loraSerial(nullptr), isInitialized(false) {
    // Constructor
}

LoRaReceiver::~LoRaReceiver() {
    if (loraSerial) {
        loraSerial->end();
    }
}

bool LoRaReceiver::setup(int rxPin, int txPin) {
    this->rxPin = rxPin;
    this->txPin = txPin;
    
    Serial.printf("Setting up LoRa receiver on pins RX:%d, TX:%d\n", rxPin, txPin);
    
    // Initialize UART for Grove-Wio-E5
    loraSerial = new HardwareSerial(1); // Use UART1
    loraSerial->begin(9600, SERIAL_8N1, rxPin, txPin);
    
    delay(2000); // Allow module to boot
    
    // Test communication
    clearSerialBuffer();
    if (!sendATCommand("AT", "OK", 3000)) {
        Serial.println("Failed to communicate with Grove-Wio-E5 module");
        return false;
    }
    
    Serial.println("Grove-Wio-E5 communication established");
    
    // Reset module to ensure clean state
    if (!reset()) {
        Serial.println("Failed to reset Grove-Wio-E5 module");
        return false;
    }
    
    // Configure LoRaWAN settings
    if (!configureLoRaWAN()) {
        Serial.println("Failed to configure LoRaWAN settings");
        return false;
    }
    
    // Join network
    if (!joinNetwork()) {
        Serial.println("Failed to join LoRaWAN network");
        return false;
    }
    
    isInitialized = true;
    Serial.println("LoRa receiver setup complete");
    return true;
}

bool LoRaReceiver::configureLoRaWAN() {
    Serial.println("Configuring LoRaWAN settings...");\n    
    // Set to LoRaWAN mode
    if (!sendATCommand("AT+MODE=LWOTAA", "OK")) {
        return false;
    }
    
    // Set region (US915 for North America, EU868 for Europe)
    // Adjust this based on your region
    if (!sendATCommand("AT+DR=US915", "OK")) {
        return false;
    }
    
    // Set data rate
    if (!sendATCommand("AT+DR=5", "OK")) {
        return false;
    }
    
    // Configure keys (these should match your transmitter)
    // Keys are defined in secrets.h for security
    String appEuiCommand = "AT+APPEUI=" + String(LORAWAN_APP_EUI);
    if (!sendATCommand(appEuiCommand, "OK")) {
        return false;
    }
    
    String appKeyCommand = "AT+APPKEY=" + String(LORAWAN_APP_KEY);
    if (!sendATCommand(appKeyCommand, "OK")) {
        return false;
    }
    
    Serial.println("LoRaWAN configuration complete");
    return true;
}

bool LoRaReceiver::joinNetwork() {
    Serial.println("Attempting to join LoRaWAN network...");
    
    // Enhanced join process with multiple attempts
    int maxAttempts = 3;
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        Serial.printf("Join attempt %d/%d\n", attempt, maxAttempts);
        
        // Clear buffer before join attempt
        clearSerialBuffer();
        
        // Attempt to join network
        if (!sendATCommand("AT+JOIN", "OK", 3000)) {
            Serial.printf("Join command failed on attempt %d\n", attempt);
            if (attempt < maxAttempts) {
                delay(5000); // Wait before retry
                continue;
            }
            return false;
        }
        
        // Wait for join confirmation (this can take up to 30 seconds)
        unsigned long startTime = millis();
        bool joinStarted = false;
        
        while (millis() - startTime < 35000) { // Extended timeout
            String response = readResponse(1000);
            
            if (response.indexOf("+JOIN: Start") >= 0) {
                joinStarted = true;
                Serial.println("Join process started...");
            } else if (response.indexOf("+JOIN: Network joined") >= 0) {
                Serial.println("Successfully joined LoRaWAN network");
                
                // Optional: Enable auto low power mode after successful join
                setAutoLowPowerMode(true);
                
                return true;
            } else if (response.indexOf("+JOIN: Join failed") >= 0) {
                Serial.printf("Join failed on attempt %d\n", attempt);
                break; // Exit inner loop to try again
            }
            delay(1000);
        }
        
        if (!joinStarted) {
            Serial.printf("Join process never started on attempt %d\n", attempt);
        } else {
            Serial.printf("Join timeout on attempt %d\n", attempt);
        }
        
        if (attempt < maxAttempts) {
            Serial.println("Waiting before next join attempt...");
            delay(10000); // Wait longer between attempts
        }
    }
    
    Serial.println("All join attempts failed");
    return false;
}

String LoRaReceiver::checkForCommand() {
    if (!isInitialized || !loraSerial) {
        return "";
    }
    
    // Check for incoming data
    if (loraSerial->available()) {
        String response = readResponse(1000);
        
        // Look for received message indicator
        int msgIndex = response.indexOf("+MSG:");
        if (msgIndex >= 0) {
            // Parse the message
            // Format: +MSG: Port=X; RX: "hexdata"
            int rxIndex = response.indexOf("RX:", msgIndex);
            if (rxIndex >= 0) {
                int startQuote = response.indexOf('"', rxIndex);
                int endQuote = response.indexOf('"', startQuote + 1);
                
                if (startQuote >= 0 && endQuote >= 0) {
                    String hexData = response.substring(startQuote + 1, endQuote);
                    
                    // Convert hex to ASCII
                    String command = "";
                    for (int i = 0; i < hexData.length(); i += 2) {
                        String hexByte = hexData.substring(i, i + 2);
                        char ascii = (char)strtol(hexByte.c_str(), NULL, 16);
                        command += ascii;
                    }
                    
                    Serial.printf("Decoded command: %s\n", command.c_str());
                    return command;
                }
            }
        }
    }
    
    return "";
}

bool LoRaReceiver::sendResponse(const String& response) {
    if (!isInitialized || !loraSerial) {
        return false;
    }
    
    // Convert ASCII to hex
    String hexData = "";
    for (int i = 0; i < response.length(); i++) {
        char hex[3];
        sprintf(hex, "%02X", (int)response[i]);
        hexData += hex;
    }
    
    // Send message
    String command = "AT+MSG=" + hexData;
    return sendATCommand(command, "OK", 10000);
}

String LoRaReceiver::getSignalQuality() {
    if (!isInitialized) {
        return "Not initialized";
    }
    
    String qualityInfo = "";
    
    // Get RSSI
    clearSerialBuffer();
    if (sendATCommand("AT+RSSI", "", 3000)) {
        String rssiResponse = readResponse(2000);
        qualityInfo += "RSSI: " + rssiResponse;
    }
    
    // Get SNR if available
    clearSerialBuffer();
    if (sendATCommand("AT+SNR", "", 3000)) {
        String snrResponse = readResponse(2000);
        if (snrResponse.length() > 0) {
            qualityInfo += ", SNR: " + snrResponse;
        }
    }
    
    // Get data rate info
    clearSerialBuffer();
    if (sendATCommand("AT+DR", "", 3000)) {
        String drResponse = readResponse(2000);
        if (drResponse.length() > 0) {
            qualityInfo += ", DR: " + drResponse;
        }
    }
    
    return qualityInfo.length() > 0 ? qualityInfo : "Error reading signal quality";
}

bool LoRaReceiver::isReady() {
    return isInitialized && sendATCommand("AT", "OK", 1000);
}

bool LoRaReceiver::reset() {
    Serial.println("Resetting Grove-Wio-E5 module...");
    
    if (!sendATCommand("AT+RESET", "", 2000)) {
        return false;
    }
    
    delay(3000); // Wait for reset to complete
    
    // Clear any boot messages
    clearSerialBuffer();
    
    // Test communication after reset
    return sendATCommand("AT", "OK", 3000);
}

// Private helper methods

bool LoRaReceiver::sendATCommand(const String& command, const String& expectedResponse, int timeout) {
    if (!loraSerial) {
        return false;
    }
    
    // Enhanced buffer clearing - wait for buffer to settle
    clearSerialBuffer();
    delay(50); // Allow any pending data to arrive
    clearSerialBuffer(); // Clear again for better reliability
    
    // Start timing measurement
    unsigned long startTime = millis();
    
    // Send command
    loraSerial->println(command);
    Serial.printf("Sent: %s\n", command.c_str());
    
    if (expectedResponse.length() == 0) {
        return true; // No response expected
    }
    
    // Wait for response with improved parsing
    String response = readResponse(timeout);
    unsigned long commandTime = millis() - startTime;
    
    Serial.printf("Received: %s (took %lu ms)\n", response.c_str(), commandTime);
    
    // More flexible response checking
    bool success = (response.indexOf(expectedResponse) >= 0) || 
                   (expectedResponse == "OK" && response.indexOf("+OK") >= 0);
    
    if (!success && expectedResponse != "") {
        Serial.printf("Command failed - expected '%s' but got '%s'\n", 
                     expectedResponse.c_str(), response.c_str());
    }
    
    return success;
}

String LoRaReceiver::readResponse(int timeout) {
    if (!loraSerial) {
        return "";
    }
    
    String response = "";
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (loraSerial->available()) {
            char c = loraSerial->read();
            response += c;
            
            // Reset timeout on new data
            startTime = millis();
        }
        delay(10);
    }
    
    response.trim();
    return response;
}

void LoRaReceiver::clearSerialBuffer() {
    if (!loraSerial) {
        return;
    }
    
    // Enhanced buffer clearing with timeout
    unsigned long startTime = millis();
    while (loraSerial->available() && (millis() - startTime < 1000)) {
        loraSerial->read();
        delay(1); // Small delay to ensure all data is read
    }
}

bool LoRaReceiver::enterLowPowerMode() {
    Serial.println("Entering LoRa low power mode...");
    return sendATCommand("AT+LOWPOWER", "OK", 3000);
}

bool LoRaReceiver::wakeUp() {
    Serial.println("Waking up LoRa module...");
    // Send any character to wake up
    loraSerial->println("AT");
    delay(100);
    return sendATCommand("AT", "OK", 3000);
}

bool LoRaReceiver::setAutoLowPowerMode(bool enable) {
    String command = "AT+LOWPOWER=AUTOMODE,";
    command += enable ? "ON" : "OFF";
    Serial.printf("Setting auto low power mode: %s\n", enable ? "ON" : "OFF");
    return sendATCommand(command, "OK", 3000);
}