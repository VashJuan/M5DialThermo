/**
 * @file lora_receiver.cpp
 * @brief LoRaWAN receiver implementation for Grove-Wio-E5
 * @version 1.0.0
 * @date 2025-12-29
 */

#include "lora_receiver.hpp"
#include "secrets.h"
#include "../../shared/protocol_common.hpp"
#include <esp_task_wdt.h>

LoRaReceiver::LoRaReceiver() : 
    loraSerial(nullptr), 
    isInitialized(false),
    currentMode(LoRaCommunicationMode::P2P)
{
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
    Serial.println("IMPORTANT: Verify physical connections:");
    Serial.println("  Grove-Wio-E5 TX --> ESP32 RX (GPIO44/D6)");
    Serial.println("  Grove-Wio-E5 RX --> ESP32 TX (GPIO43/D7)");
    Serial.println("  Grove-Wio-E5 VCC --> 3.3V");
    Serial.println("  Grove-Wio-E5 GND --> GND");
    
    // Initialize UART for Grove-Wio-E5
    loraSerial = new HardwareSerial(1); // Use UART1
    
    Serial.println("Waiting for Grove-Wio-E5 and M5Dial to power up and stabilize...");
    Serial.printf("Initialization timeout: %d seconds\n", LORA_INIT_TIMEOUT_MS / 1000);
    delay(3000); // Give module time to fully boot after power-on
    
    bool communicationEstablished = false;
    unsigned long initStartTime = millis();
    
#if LORA_DISABLE_BAUD_SEARCH
    // Use fixed baud rate - no search
    Serial.printf("Using fixed baud rate: %d (baud search disabled)\n", LORA_FIXED_BAUD_RATE);
    loraSerial->begin(LORA_FIXED_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
    delay(2000); // Wait for module to stabilize
    
    // Try to connect with extended timeout for M5Dial to come online
    int attempt = 0;
    while (!communicationEstablished && (millis() - initStartTime < LORA_INIT_TIMEOUT_MS)) {
        attempt++;
        Serial.printf("Connection attempt %d (elapsed: %lu ms)...\n", 
                     attempt, millis() - initStartTime);
        esp_task_wdt_reset(); // Reset watchdog during attempts
        
        // Send wake-up sequence
        loraSerial->println();
        delay(200);
        loraSerial->println("AT");
        delay(200);
        clearSerialBuffer();
        
        if (sendATCommand("AT", "OK", 3000)) {
            Serial.printf("SUCCESS! Module responding at %d baud\n", LORA_FIXED_BAUD_RATE);
            communicationEstablished = true;
            break;
        }
        
        // Check if ANY data was received
        clearSerialBuffer();
        loraSerial->println("AT");
        delay(500);
        if (loraSerial->available()) {
            Serial.print("  Received data: ");
            while (loraSerial->available()) {
                Serial.printf("0x%02X ", loraSerial->read());
            }
            Serial.println("\n  (May indicate wrong baud rate or connection issue)");
        } else {
            Serial.println("  No response - waiting for M5Dial...");
        }
        
        delay(2000); // Wait longer between attempts for M5Dial to initialize
    }
#else
    // Try multiple baud rates - Grove-Wio-E5 can be 9600, 19200, or 115200
    const int baudRates[] = {19200, 9600, 115200};
    
    for (int baud : baudRates) {
        if (millis() - initStartTime >= LORA_INIT_TIMEOUT_MS) {
            Serial.println("Initialization timeout reached");
            break;
        }
        
        Serial.printf("\nTrying baud rate: %d\n", baud);
        esp_task_wdt_reset(); // Reset watchdog before trying new baud rate
        
        if (baud != 19200) {
            loraSerial->end(); // End previous attempt (skip for first attempt)
            delay(500);
        }
        loraSerial->begin(baud, SERIAL_8N1, rxPin, txPin);
        
        delay(2000); // Wait for module to stabilize
        
        // Send wake-up sequence
        loraSerial->println();
        delay(200);
        loraSerial->println("AT");
        delay(200);
        clearSerialBuffer();
        
        // Test with multiple AT command attempts, respecting overall timeout
        for (int attempt = 1; attempt <= 5; attempt++) {
            if (millis() - initStartTime >= LORA_INIT_TIMEOUT_MS) {
                Serial.println("Initialization timeout reached");
                break;
            }
            
            Serial.printf("  Attempt %d at %d baud (elapsed: %lu ms)...\n", 
                         attempt, baud, millis() - initStartTime);
            esp_task_wdt_reset(); // Reset watchdog during attempts
            
            if (sendATCommand("AT", "OK", 3000)) {
                Serial.printf("SUCCESS! Module responding at %d baud\n", baud);
                communicationEstablished = true;
                break;
            }
            
            // Check if ANY data was received
            clearSerialBuffer();
            loraSerial->println("AT");
            delay(500);
            if (loraSerial->available()) {
                Serial.print("  Received garbage data: ");
                while (loraSerial->available()) {
                    Serial.printf("0x%02X ", loraSerial->read());
                }
                Serial.println("\n  (Wrong baud rate or connection issue)");
            } else {
                Serial.println("  No response - waiting for M5Dial...");
            }
            
            delay(2000); // Wait longer between attempts
        }
        
        if (communicationEstablished) break;
    }
#endif
    
    if (!communicationEstablished) {
        Serial.println("\n========================================");
        Serial.printf("FAILED: Could not communicate with module after %lu seconds!\n", 
                     (millis() - initStartTime) / 1000);
        Serial.println("Troubleshooting steps:");
        Serial.println("1. Ensure M5Dial is powered on and initialized");
        Serial.println("2. Verify RX/TX are NOT swapped");
        Serial.println("3. Check 3.3V power with multimeter");
        Serial.println("4. Ensure Grove-Wio-E5 has antenna attached");
#if LORA_DISABLE_BAUD_SEARCH
        Serial.printf("5. Verify both devices are using %d baud\n", LORA_FIXED_BAUD_RATE);
#else
        Serial.println("5. Try enabling fixed baud rate mode (LORA_DISABLE_BAUD_SEARCH)");
#endif
        Serial.println("========================================");
        return false;
    }
    
    Serial.println("Grove-Wio-E5 communication established");
    
    // Disable echo to prevent command echoing
    Serial.println("Disabling echo mode...");
    clearSerialBuffer();
    delay(100);
    if (sendATCommand("ATE0", "OK", 2000)) {
        Serial.println("Echo disabled successfully");
    } else {
        Serial.println("Warning: Could not disable echo (continuing anyway)");
    }
    
    // Reset module to ensure clean state
    if (!reset()) {
        Serial.println("Failed to reset Grove-Wio-E5 module");
        return false;
    }
    
    // Try P2P mode first (default)
    currentMode = LoRaCommunicationMode::P2P;
    if (configureP2P()) {
        Serial.println("P2P mode configured successfully");
        isInitialized = true;
        return true;
    }
    
    Serial.println("P2P configuration failed, falling back to LoRaWAN...");
    
    // Fall back to LoRaWAN mode
    currentMode = LoRaCommunicationMode::LoRaWAN;
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

bool LoRaReceiver::configureP2P()
{
    Serial.println("Configuring P2P mode...");
    
    // Enter TEST mode for P2P communication
    if (!sendATCommand("AT+MODE=TEST", "OK")) {
        Serial.println("Failed to enter TEST mode");
        return false;
    }
    
    // Configure RF parameters for P2P using constants from protocol_common.hpp
    String rfConfigCommand = "AT+TEST=RFCFG," + 
                           String(P2P_FREQUENCY) + "000000," +  // Convert MHz to Hz
                           P2P_SPREADING_FACTOR + "," +
                           P2P_BANDWIDTH + "," +
                           P2P_CODING_RATE + "," +
                           P2P_PREAMBLE_LENGTH + "," +
                           P2P_TX_POWER;
    
    if (!sendATCommand(rfConfigCommand, "OK")) {
        Serial.println("Failed to configure P2P RF parameters");
        return false;
    }
    
    Serial.println("P2P mode configured successfully");
    Serial.printf("Frequency: %d MHz, SF: %s, BW: %s, CR: %s, Power: %s dBm\n",
                  P2P_FREQUENCY, P2P_SPREADING_FACTOR, P2P_BANDWIDTH, 
                  P2P_CODING_RATE, P2P_TX_POWER);
    
    return true;
}

bool LoRaReceiver::configureLoRaWAN() {
    Serial.println("Configuring LoRaWAN settings...");
    
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

bool LoRaReceiver::sendP2PMessage(const String &message)
{
    // Convert message to hex format
    String hexMessage = ProtocolHelper::asciiToHex(message);
    
    // Send P2P message using AT+TEST=TXLRPKT
    String command = "AT+TEST=TXLRPKT,\"" + hexMessage + "\"";
    
    if (!sendATCommand(command, "TX DONE", 3000)) {
        Serial.println("P2P transmission failed");
        return false;
    }
    
    Serial.printf("P2P message sent: %s (hex: %s)\n", message.c_str(), hexMessage.c_str());
    return true;
}

String LoRaReceiver::receiveP2PMessage(int timeout)
{
    // Enter receive mode
    if (!sendATCommand("AT+TEST=RXLRPKT", "RX DONE", timeout)) {
        return "";
    }
    
    // Wait for received message
    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        String response = readResponse(100);
        
        // Look for received data in format: +TEST: RX "hexdata"
        int rxIndex = response.indexOf("+TEST: RX ");
        if (rxIndex >= 0) {
            int startQuote = response.indexOf('"', rxIndex);
            int endQuote = response.indexOf('"', startQuote + 1);
            
            if (startQuote >= 0 && endQuote >= 0) {
                String hexData = response.substring(startQuote + 1, endQuote);
                String decodedMessage = ProtocolHelper::hexToAscii(hexData);
                Serial.printf("P2P message received: %s (hex: %s)\n", 
                            decodedMessage.c_str(), hexData.c_str());
                return decodedMessage;
            }
        }
        
        delay(10);
    }
    
    Serial.println("No P2P message received within timeout");
    return "";
}

bool LoRaReceiver::enterP2PReceiveMode()
{
    return sendATCommand("AT+TEST=RXLRPKT", "RX DONE", 1000);
}

String LoRaReceiver::checkForCommand() {
    if (!isInitialized || !loraSerial) {
        return "";
    }
    
    // Use different methods based on current communication mode
    if (currentMode == LoRaCommunicationMode::P2P) {
        // P2P mode: Check for P2P messages
        return receiveP2PMessage(100); // Quick check, don't block long
    } else {
        // LoRaWAN mode: Check for downlink messages
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
                        String command = ProtocolHelper::hexToAscii(hexData);
                        
                        Serial.printf("LoRaWAN command received: %s\n", command.c_str());
                        return command;
                    }
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
    
    // Use different methods based on current communication mode
    if (currentMode == LoRaCommunicationMode::P2P) {
        // P2P mode: Send direct P2P message
        return sendP2PMessage(response);
    } else {
        // LoRaWAN mode: Send downlink message
        String hexData = ProtocolHelper::asciiToHex(response);
        String command = "AT+MSG=" + hexData;
        return sendATCommand(command, "OK", 10000);
    }
}

String LoRaReceiver::getSignalQuality() {
    if (!isInitialized) {
        return "Not initialized";
    }
    
    // Note: Some Grove-Wio-E5 modules don't respond to these commands in P2P mode
    // Use shorter timeouts to prevent watchdog issues
    String qualityInfo = "";
    
    // Try to get RSSI (may not be supported in P2P mode)
    clearSerialBuffer();
    if (sendATCommand("AT+RSSI", "", 1000)) {
        String rssiResponse = readResponse(500);
        if (rssiResponse.length() > 0) {
            qualityInfo += "RSSI: " + rssiResponse;
        }
    }
    
    // Try to get SNR if available (may not be supported in P2P mode)
    clearSerialBuffer();
    if (sendATCommand("AT+SNR", "", 1000)) {
        String snrResponse = readResponse(500);
        if (snrResponse.length() > 0 && qualityInfo.length() > 0) {
            qualityInfo += ", SNR: " + snrResponse;
        } else if (snrResponse.length() > 0) {
            qualityInfo += "SNR: " + snrResponse;
        }
    }
    
    return qualityInfo.length() > 0 ? qualityInfo : "Signal monitoring not available in current mode";
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
    
    // Handle echo: module may echo the command before responding
    // Check if the expected response exists anywhere in the full response
    // This handles cases where format is "AT\r\nOK\r\n" or "AT\r\n+AT: OK\r\n"
    bool success = (response.indexOf(expectedResponse) >= 0) || 
                   (expectedResponse == "OK" && (response.indexOf("+OK") >= 0 || 
                                                  response.indexOf("\nOK") >= 0 ||
                                                  response.indexOf("\r\nOK") >= 0 ||
                                                  response.indexOf("+AT: OK") >= 0 ||
                                                  (response.indexOf("OK") >= 0 && response.length() > command.length()))) ||
                   (response.length() > 0 && expectedResponse.length() > 0 && 
                    response.indexOf("+") == 0); // Any "+XXX" response is valid acknowledgment
    
    if (!success && expectedResponse != "") {
        Serial.printf("Command failed - expected '%s' but got '%s'\n", 
                     expectedResponse.c_str(), response.c_str());
        // Print hex dump for debugging if we got data
        if (response.length() > 0) {
            Serial.print("  Received data: ");
            for (unsigned int i = 0; i < response.length() && i < 50; i++) {
                Serial.printf("0x%02X ", (unsigned char)response.charAt(i));
            }
            Serial.println();
            // Check if this looks like echo
            String responseUpper = response;
            responseUpper.toUpperCase();
            String commandUpper = command;
            commandUpper.toUpperCase();
            if (responseUpper.startsWith(commandUpper) && response.indexOf("OK") < 0) {
                Serial.println("  (Echo received but no OK - module may need reset or longer timeout)");
            } else if (response.length() > 0 && response.indexOf(command) < 0 && response.indexOf(expectedResponse) < 0) {
                Serial.println("  (Unexpected response - may indicate wrong baud rate)");
            }
        } else {
            Serial.println("  No response received - check connections and power");
        }
    }
    
    return success;
}

String LoRaReceiver::readResponse(int timeout) {
    if (!loraSerial) {
        return "";
    }
    
    String response = "";
    unsigned long startTime = millis();
    unsigned long lastDataTime = millis();
    
    while (millis() - startTime < timeout) {
        if (loraSerial->available()) {
            char c = loraSerial->read();
            response += c;
            lastDataTime = millis(); // Reset data timeout
        } else if (response.length() > 0 && (millis() - lastDataTime > 200)) {
            // If we've received data and there's been 200ms of silence, consider response complete
            break;
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

LoRaCommunicationMode LoRaReceiver::getCurrentMode()
{
    return currentMode;
}

bool LoRaReceiver::switchMode(LoRaCommunicationMode mode)
{
    if (!isInitialized) {
        Serial.println("Receiver not initialized");
        return false;
    }
    
    if (currentMode == mode) {
        Serial.printf("Already in %s mode\n", mode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
        return true;
    }
    
    Serial.printf("Switching from %s to %s mode\n", 
                  currentMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN",
                  mode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
    
    bool success = false;
    if (mode == LoRaCommunicationMode::P2P) {
        success = configureP2P();
    } else {
        success = configureLoRaWAN() && joinNetwork();
    }
    
    if (success) {
        currentMode = mode;
        Serial.printf("Successfully switched to %s mode\n", 
                      mode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
    } else {
        Serial.printf("Failed to switch to %s mode\n",
                     mode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
    }
    
    return success;
}

String LoRaReceiver::checkForCommandWithFallback()
{
    if (!isInitialized) {
        Serial.println("Receiver not initialized");
        return "";
    }
    
    Serial.printf("Checking for command in %s mode\n", 
                  currentMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
    
    // Try current mode first
    String command = checkForCommand();
    if (command.length() > 0) {
        return command;
    }
    
    // If current mode didn't receive anything, try the other mode
    LoRaCommunicationMode fallbackMode = (currentMode == LoRaCommunicationMode::P2P) ? 
                                         LoRaCommunicationMode::LoRaWAN : 
                                         LoRaCommunicationMode::P2P;
    
    Serial.printf("No command in primary mode, trying fallback mode: %s\n", 
                  fallbackMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
    
    if (switchMode(fallbackMode)) {
        command = checkForCommand();
        if (command.length() > 0) {
            Serial.printf("Fallback successful with %s mode\n", 
                         fallbackMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
            return command;
        }
    }
    
    // No command received in either mode
    return "";
}