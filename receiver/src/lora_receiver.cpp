/**
 * @file lora_receiver.cpp
 * @brief LoRaWAN receiver implementation for Grove-Wio-E5
 * @version 1.0.0
 * @date 2025-12-29
 */

#include "lora_receiver.hpp"

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
    // NOTE: You'll need to set these to match your network and transmitter
    // These are example values - replace with your actual keys
    if (!sendATCommand("AT+APPEUI=0000000000000000", "OK")) {
        return false;
    }
    
    if (!sendATCommand("AT+APPKEY=00000000000000000000000000000000", "OK")) {
        return false;
    }
    
    Serial.println("LoRaWAN configuration complete");
    return true;
}

bool LoRaReceiver::joinNetwork() {
    Serial.println("Attempting to join LoRaWAN network...");
    
    // Attempt to join network
    if (!sendATCommand("AT+JOIN", "OK", 2000)) {
        return false;
    }
    
    // Wait for join confirmation (this can take up to 30 seconds)
    unsigned long startTime = millis();
    while (millis() - startTime < 30000) {
        String response = readResponse(1000);
        if (response.indexOf("+JOIN: Network joined") >= 0) {
            Serial.println("Successfully joined LoRaWAN network");
            return true;
        } else if (response.indexOf("+JOIN: Join failed") >= 0) {
            Serial.println("Failed to join LoRaWAN network");
            return false;
        }
        delay(1000);
    }
    
    Serial.println("Network join timeout");
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
    
    // Get RSSI and SNR
    if (sendATCommand("AT+RSSI", "", 3000)) {
        String response = readResponse(3000);
        return response;
    }
    
    return "Error reading signal quality";
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
    
    // Clear buffer before sending command
    clearSerialBuffer();
    
    // Send command
    loraSerial->println(command);
    Serial.printf("Sent: %s\n", command.c_str());
    
    if (expectedResponse.length() == 0) {
        return true; // No response expected
    }
    
    // Wait for response
    String response = readResponse(timeout);
    Serial.printf("Received: %s\n", response.c_str());
    
    return response.indexOf(expectedResponse) >= 0;
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
    
    while (loraSerial->available()) {
        loraSerial->read();
    }
}