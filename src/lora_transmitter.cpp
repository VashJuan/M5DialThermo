/**
 * @file lora_transmitter.cpp
 * @brief LoRaWAN transmitter implementation for Grove-Wio-E5
 * @version 1.0.0
 * @date 2025-12-30
 */

#include "lora_transmitter.hpp"
#include <esp_task_wdt.h>

LoRaTransmitter::LoRaTransmitter() : 
    loraSerial(nullptr), 
    isInitialized(false),
    currentMode(LoRaCommunicationMode::P2P),
    lastTransmissionTime(0),
    lastAckTime(0),
    successfulTransmissions(0),
    failedTransmissions(0),
    totalRetries(0),
    lastError("")
{
    // Constructor
}

LoRaTransmitter::~LoRaTransmitter()
{
    if (loraSerial) {
        loraSerial->end();
    }
}

bool LoRaTransmitter::setup(int rxPin, int txPin, const LoRaWANConfig &loraConfig)
{
    this->rxPin = rxPin;
    this->txPin = txPin;
    this->config = loraConfig;
    
    Serial.printf("Setting up LoRa transmitter on pins RX:%d, TX:%d\n", rxPin, txPin);
    
    // Initialize UART for Grove-Wio-E5
    loraSerial = new HardwareSerial(1); // Use UART1
    
    Serial.println("Initializing LoRa module - patient connection mode enabled");
    Serial.printf("Initialization timeout: %d seconds\n", LORA_TX_INIT_TIMEOUT_MS / 1000);
    delay(2000); // Allow module to boot
    
    bool communicationEstablished = false;
    unsigned long initStartTime = millis();
    
#if LORA_TX_DISABLE_BAUD_SEARCH
    // Use fixed baud rate - no search
    Serial.printf("Using fixed baud rate: %d (baud search disabled)\n", LORA_TX_FIXED_BAUD_RATE);
    loraSerial->begin(LORA_TX_FIXED_BAUD_RATE, SERIAL_8N1, rxPin, txPin);
    
    // Give module MUCH more time to fully boot - some modules need 5+ seconds
    // Break up delay with watchdog resets to prevent timeout
    Serial.println("Waiting for module to fully boot (5 seconds)...");
    for (int i = 0; i < 10; i++) {
        delay(500);
        esp_task_wdt_reset();
    }
    
    // Send multiple wake-up commands to ensure module is responsive
    Serial.println("Sending wake-up sequence...");
    for (int i = 0; i < 5; i++) {
        clearSerialBuffer();
        loraSerial->println();
        delay(200);
    }
    clearSerialBuffer();
    delay(500);
    
    // Try to connect with extended timeout - no rush
    int attempt = 0;
    while (!communicationEstablished && (millis() - initStartTime < LORA_TX_INIT_TIMEOUT_MS)) {
        attempt++;
        Serial.printf("Connection attempt %d (elapsed: %lu ms)...\n", 
                     attempt, millis() - initStartTime);
        esp_task_wdt_reset(); // Reset watchdog during attempts
        
        // Send wake-up bytes in case module is in low-power auto mode
        // Based on andresoliva/LoRa-E5 library wake-up sequence
        clearSerialBuffer();
        loraSerial->write(0xFF);
        loraSerial->write(0xFF);
        loraSerial->write(0xFF);
        loraSerial->write(0xFF);
        delay(100);
        clearSerialBuffer();
        
        if (sendATCommand("AT", "OK", 2000)) {
            Serial.printf("SUCCESS! Module responding at %d baud\n", LORA_TX_FIXED_BAUD_RATE);
            communicationEstablished = true;
            break;
        }
        delay(2000); // Patient delay between attempts
    }
#else
    // Try multiple baud rates - Grove-Wio-E5 can be 9600, 19200, or 115200
    const int baudRates[] = {19200, 9600, 115200};
    
    for (int baud : baudRates) {
        if (millis() - initStartTime >= LORA_TX_INIT_TIMEOUT_MS) {
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
        
        delay(1000); // Wait for module to stabilize
        
        // Test communication with multiple patient attempts
        clearSerialBuffer();
        for (int attempt = 1; attempt <= 5; attempt++) {
            if (millis() - initStartTime >= LORA_TX_INIT_TIMEOUT_MS) {
                Serial.println("Initialization timeout reached");
                break;
            }
            
            Serial.printf("  Attempt %d at %d baud (elapsed: %lu ms)...\n", 
                         attempt, baud, millis() - initStartTime);
            esp_task_wdt_reset(); // Reset watchdog during attempts
            
            if (sendATCommand("AT", "OK", 2000)) {
                Serial.printf("SUCCESS! Module responding at %d baud\n", baud);
                communicationEstablished = true;
                break;
            }
            delay(2000); // Patient delay between attempts
        }
        
        if (communicationEstablished) break;
    }
#endif
    
    if (!communicationEstablished) {
        lastError = "Failed to communicate with Grove-Wio-E5 module after " + 
                   String((millis() - initStartTime) / 1000) + " seconds";
        Serial.println(lastError);
#if LORA_TX_DISABLE_BAUD_SEARCH
        Serial.printf("Note: Using fixed baud rate %d - verify receiver uses same baud\n", 
                     LORA_TX_FIXED_BAUD_RATE);
#endif
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
        lastError = "Failed to reset Grove-Wio-E5 module";
        Serial.println(lastError);
        return false;
    }
    
    // Try P2P mode first (default)
    currentMode = LoRaCommunicationMode::P2P;
    if (configureP2P()) {
        Serial.println("P2P mode configured successfully");
        isInitialized = true;
        clearStatistics();
        return true;
    }
    
    Serial.println("P2P configuration failed, falling back to LoRaWAN...");
    
    // Fall back to LoRaWAN mode
    currentMode = LoRaCommunicationMode::LoRaWAN;
    if (!configureLoRaWAN()) {
        lastError = "Failed to configure LoRaWAN settings";
        Serial.println(lastError);
        return false;
    }
    
    // Join network for LoRaWAN
    if (!joinNetwork()) {
        lastError = "Failed to join LoRaWAN network";
        Serial.println(lastError);
        return false;
    }
    
    isInitialized = true;
    Serial.println("LoRa transmitter setup complete (LoRaWAN mode)");
    
    // Clear statistics
    clearStatistics();
    
    return true;
}

bool LoRaTransmitter::configureP2P()
{
    Serial.println("Configuring P2P mode...");
    
    // Enter TEST mode for P2P communication
    // Success response is "+MODE: TEST", not "OK"
    if (!sendATCommand("AT+MODE=TEST", "TEST")) {
        Serial.println("Failed to enter TEST mode");
        return false;
    }
    
    // Configure RF parameters for P2P
    String rfConfigCommand = "AT+TEST=RFCFG," + 
                           String(P2P_FREQUENCY) + "," +
                           String(P2P_SPREADING_FACTOR) + "," +
                           String(P2P_BANDWIDTH) + "," +
                           String(P2P_CODING_RATE) + "," +
                           String(P2P_PREAMBLE_LENGTH) + "," +
                           String(P2P_POWER);
    
    // Success response is "+TEST: RFCFG ...", not "OK"
    if (!sendATCommand(rfConfigCommand, "RFCFG")) {
        Serial.println("Failed to configure P2P RF parameters");
        return false;
    }
    
    Serial.println("P2P mode configured successfully");
    Serial.printf("Frequency: %lu Hz, SF: %d, BW: %d, CR: %d, Power: %d dBm\n",
                  P2P_FREQUENCY, P2P_SPREADING_FACTOR, P2P_BANDWIDTH, 
                  P2P_CODING_RATE, P2P_POWER);
    
    return true;
}

bool LoRaTransmitter::configureLoRaWAN()
{
    Serial.println("Configuring LoRaWAN transmitter settings...");
    
    // Set to LoRaWAN mode (OTAA if configured, otherwise ABP)
    // Success response is "+MODE: LWOTAA" or "+MODE: LWABP", not "OK"
    String modeCommand = config.otaa ? "AT+MODE=LWOTAA" : "AT+MODE=LWABP";
    String expectedMode = config.otaa ? "LWOTAA" : "LWABP";
    if (!sendATCommand(modeCommand, expectedMode)) {
        return false;
    }
    
    // Set region
    // Success response is "+DR: US915", not "OK"
    String regionCommand = "AT+DR=" + config.region;
    if (!sendATCommand(regionCommand, config.region)) {
        return false;
    }
    
    // Set data rate
    // Success response contains "DR" confirmation, not "OK"
    String drCommand = "AT+DR=" + String(config.dataRate);
    if (!sendATCommand(drCommand, "DR")) {
        return false;
    }
    
    // Configure OTAA keys if using OTAA
    if (config.otaa) {
        // Set AppEUI
        String appEuiCommand = "AT+ID=APPEUI," + config.appEUI;
        if (!sendATCommand(appEuiCommand, "OK")) {
            return false;
        }
        
        // Set AppKey
        String appKeyCommand = "AT+KEY=APPKEY," + config.appKey;
        if (!sendATCommand(appKeyCommand, "OK")) {
            return false;
    }
    }
    
    // Set device class
    if (!sendATCommand("AT+CLASS=A", "OK")) {
        return false;
    }
    
    // Set confirmed/unconfirmed uplinks
    String confirmedCommand = "AT+CFM=" + String(config.confirmUplinks);
    if (!sendATCommand(confirmedCommand, "OK")) {
        return false;
    }
    
    // Set transmit power
    String powerCommand = "AT+POWER=" + String(config.transmitPower);
    if (!sendATCommand(powerCommand, "OK")) {
        return false;
    }
    
    // Enable/disable ADR
    String adrCommand = "AT+ADR=" + (config.adaptiveDataRate ? String("ON") : String("OFF"));
    if (!sendATCommand(adrCommand, "OK")) {
        return false;
    }
    
    Serial.println("LoRaWAN transmitter configuration complete");
    return true;
}

bool LoRaTransmitter::joinNetwork()
{
    if (!config.otaa) {
        Serial.println("Using ABP mode - no join required");
        return true;
    }
    
    Serial.println("Attempting to join LoRaWAN network...");
    
    // Enhanced join process with multiple attempts (inspired by Grove-Wio-E5 examples)
    int maxAttempts = 3;
    for (int attempt = 1; attempt <= maxAttempts; attempt++) {
        Serial.printf("Join attempt %d/%d\n", attempt, maxAttempts);
        
        // Clear buffer before join attempt
        clearSerialBuffer();
        
        // Attempt to join network with timing measurement
        unsigned long joinTime = 0;
        if (!sendATCommandWithTiming("AT+JOIN", "OK", 3000, joinTime)) {
            Serial.printf("Join command failed on attempt %d (took %lu ms)\n", attempt, joinTime);
            if (attempt < maxAttempts) {
                delay(5000); // Wait before retry
                continue;
            }
            return false;
        }
        
        // Wait for join confirmation (this can take up to 30 seconds)
        unsigned long startTime = millis();
        bool joinStarted = false;
        
        while (millis() - startTime < LORAWAN_JOIN_TIMEOUT) {
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

bool LoRaTransmitter::sendP2PMessage(const String &message)
{
    // Convert message to hex format
    String hexMessage = ProtocolHelper::asciiToHex(message);
    
    // Send P2P message using AT+TEST=TXLRPKT
    // Module first echoes command, then sends TX DONE after transmission
    String command = "AT+TEST=TXLRPKT,\"" + hexMessage + "\"";
    
    // Use extended timeout to wait for actual TX DONE response (not just echo)
    if (!sendATCommand(command, "TX DONE", 5000)) {
        Serial.println("P2P transmission failed");
        return false;
    }
    
    Serial.printf("P2P message sent: %s (hex: %s)\n", message.c_str(), hexMessage.c_str());
    return true;
}

String LoRaTransmitter::receiveP2PMessage(int timeout)
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

bool LoRaTransmitter::enterP2PReceiveMode()
{
    return sendATCommand("AT+TEST=RXLRPKT", "RX DONE", 1000);
}

String LoRaTransmitter::sendCommand(const String &command, uint8_t port, bool confirmed, int maxRetries)
{
    if (!isInitialized) {
        lastError = "Transmitter not initialized";
        Serial.println(lastError);
        return "";
    }
    
    Serial.printf("Sending command: %s (mode: %s, port: %d, confirmed: %s)\n", 
                  command.c_str(), 
                  currentMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN",
                  port, confirmed ? "yes" : "no");
    
    // Validate command
    if (!ProtocolHelper::isValidCommand(command)) {
        lastError = "Invalid command: " + command;
        Serial.println(lastError);
        failedTransmissions++;
        return "";
    }
    
    // Use appropriate communication method based on current mode
    if (currentMode == LoRaCommunicationMode::P2P) {
        // P2P communication - simple direct transmission
        for (int attempt = 0; attempt <= maxRetries; attempt++) {
            if (attempt > 0) {
                Serial.printf("P2P retry attempt %d/%d\n", attempt, maxRetries);
                totalRetries++;
                delay(1000); // Wait before retry
            }
            
            lastTransmissionTime = millis();
            
            if (sendP2PMessage(command)) {
                if (confirmed) {
                    // Wait for response in P2P mode
                    String response = receiveP2PMessage(P2P_RX_TIMEOUT);
                    if (response.length() > 0 && ProtocolHelper::isValidResponse(response)) {
                        lastAckTime = millis();
                        successfulTransmissions++;
                        return response;
                    }
                } else {
                    // Unconfirmed P2P message
                    successfulTransmissions++;
                    return "SENT";
                }
            }
        }
        
        lastError = "P2P transmission failed after " + String(maxRetries + 1) + " attempts";
        Serial.println(lastError);
        failedTransmissions++;
        return "";
    } else {
        // LoRaWAN communication (existing implementation)
        // Create hex message
        String hexMessage = createHexMessage(command, port);
        
        // Attempt transmission with retries
        for (int attempt = 0; attempt <= maxRetries; attempt++) {
            if (attempt > 0) {
                Serial.printf("LoRaWAN retry attempt %d/%d\n", attempt, maxRetries);
                totalRetries++;
                delay(2000); // Wait before retry
            }
            
            // Record transmission start time
            lastTransmissionTime = millis();
            
            // Send message
            if (sendMessage(hexMessage, confirmed)) {
                // Wait for response if confirmed message
                if (confirmed) {
                    unsigned long responseStartTime = millis();
                    while (millis() - responseStartTime < LORAWAN_RX_TIMEOUT) {
                        if (loraSerial->available()) {
                            String response = readResponse(1000);
                            
                            // Look for downlink message indicator
                            int msgIndex = response.indexOf("+MSG:");
                            if (msgIndex >= 0) {
                                // Parse the response message
                                int rxIndex = response.indexOf("RX:", msgIndex);
                                if (rxIndex >= 0) {
                                    int startQuote = response.indexOf('"', rxIndex);
                                    int endQuote = response.indexOf('"', startQuote + 1);
                                    
                                    if (startQuote >= 0 && endQuote >= 0) {
                                        String hexData = response.substring(startQuote + 1, endQuote);
                                        String decodedResponse = ProtocolHelper::hexToAscii(hexData);
                                        
                                        Serial.printf("LoRaWAN response received: %s\n", decodedResponse.c_str());
                                        
                                        // Validate response
                                        if (ProtocolHelper::isValidResponse(decodedResponse)) {
                                            lastAckTime = millis();
                                            successfulTransmissions++;
                                            return decodedResponse;
                                        }
                                    }
                                }
                            }
                        }
                        delay(100);
                    }
                    
                    Serial.println("No LoRaWAN response received within timeout");
                } else {
                    // Unconfirmed message - consider successful if sent
                    successfulTransmissions++;
                    return "SENT"; // Indicate message was sent (no response expected)
                }
            }
        }
        
        lastError = "LoRaWAN transmission failed after " + String(maxRetries + 1) + " attempts";
        Serial.println(lastError);
        failedTransmissions++;
        return "";
    }
}

bool LoRaTransmitter::ping()
{
    String response = sendCommand(CMD_PING, LORAWAN_PORT_PING, true, 2);
    return (response == RESP_PONG);
}

String LoRaTransmitter::requestStatus()
{
    return sendCommand(CMD_STATUS_REQUEST, LORAWAN_PORT_STATUS, true, 2);
}

String LoRaTransmitter::getSignalQuality()
{
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

bool LoRaTransmitter::isReady()
{
    return isInitialized && sendATCommand("AT", "OK", 1000);
}

bool LoRaTransmitter::reset()
{
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

bool LoRaTransmitter::sendATCommand(const String &command, const String &expectedResponse, int timeout)
{
    if (!loraSerial) {
        return false;
    }
    
    // Enhanced buffer clearing (inspired by Grove-Wio-E5 examples)
    clearSerialBuffer();
    delay(50); // Allow any pending data to arrive
    clearSerialBuffer(); // Clear again for better reliability
    
    // Send command
    loraSerial->println(command);
    Serial.printf("TX: %s\n", command.c_str());
    
    if (expectedResponse.length() == 0) {
        return true; // No response expected
    }
    
    // Wait for response
    String response = readResponse(timeout);
    Serial.printf("RX: %s\n", response.c_str());
    
    // Handle echo: module may echo the command before responding
    // Check if the expected response exists anywhere in the full response
    // This handles cases where format is "AT\r\nOK\r\n" or "AT\r\n+AT: OK\r\n"
    bool success = (response.indexOf(expectedResponse) >= 0) || 
                   (expectedResponse == "OK" && (response.indexOf("+OK") >= 0 || 
                                                  response.indexOf("\nOK") >= 0 ||
                                                  response.indexOf("\r\nOK") >= 0 ||
                                                  response.indexOf("+AT: OK") >= 0 ||
                                                  (response.indexOf("OK") >= 0 && response.length() > command.length())));
    
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

bool LoRaTransmitter::sendATCommandWithTiming(const String &command, const String &expectedResponse, int timeout, unsigned long &commandTime)
{
    unsigned long startTime = millis();
    bool result = sendATCommand(command, expectedResponse, timeout);
    commandTime = millis() - startTime;
    return result;
}

String LoRaTransmitter::readResponse(int timeout)
{
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
        } else if (response.length() > 0) {
            // For short responses during init (like echoed AT), wait longer for the actual response
            // Only break if we've waited at least 500ms after last data
            unsigned long silenceTime = millis() - lastDataTime;
            if (silenceTime > 500) {
                // If response looks incomplete (just echo with no OK/DONE), wait longer
                // TX DONE can take time after echo is received
                if (response.length() <= 10 && response.indexOf("OK") < 0 && response.indexOf("DONE") < 0 && silenceTime < 2000) {
                    // Keep waiting for TX DONE or other completion messages
                    delay(10);
                    continue;
                }
                // Got enough silence, response is complete
                break;
            }
        }
        delay(10);
    }
    
    response.trim();
    return response;
}

void LoRaTransmitter::clearSerialBuffer()
{
    if (!loraSerial) {
        return;
    }
    
    // Enhanced buffer clearing with timeout (inspired by Grove-Wio-E5 examples)
    unsigned long startTime = millis();
    while (loraSerial->available() && (millis() - startTime < 1000)) {
        loraSerial->read();
        delay(1); // Small delay to ensure all data is read
    }
}

String LoRaTransmitter::createHexMessage(const String &command, uint8_t port)
{
    // Use protocol helper to create the message
    return ProtocolHelper::createMessage(command, port);
}

bool LoRaTransmitter::sendMessage(const String &hexMessage, bool confirmed)
{
    // Construct AT command for sending hex message
    String command = "AT+CMSGHEX=\"" + hexMessage + "\"";
    
    // Use enhanced timing measurements (inspired by Grove-Wio-E5 time measures example)
    unsigned long txTime = 0, ackTime = 0;
    
    if (!sendATCommandWithTiming(command, "Done", LORAWAN_TX_TIMEOUT, txTime)) {
        return false;
    }
    
    Serial.printf("Message sent in %lu ms\n", txTime);
    
    // If confirmed message, wait for transmission completion and ACK
    if (confirmed) {
        if (waitForTransmissionComplete(txTime, ackTime)) {
            Serial.printf("ACK received in %lu ms\n", ackTime);
            return true;
        } else {
            Serial.println("No ACK received or transmission failed");
            return false;
        }
    }
    
    return true; // Unconfirmed message sent successfully
}

bool LoRaTransmitter::waitForTransmissionComplete(unsigned long &txTime, unsigned long &ackTime)
{
    unsigned long startTime = millis();
    bool transmissionStarted = false;
    bool waitingForAck = false;
    unsigned long ackStartTime = 0;
    
    while (millis() - startTime < LORAWAN_TX_TIMEOUT + LORAWAN_RX_TIMEOUT) {
        String response = readResponse(1000);
        
        // Look for transmission start
        if (response.indexOf("Start") >= 0) {
            transmissionStarted = true;
            txTime = millis() - startTime;
        }
        
        // Look for "Wait ACK" indication
        if (response.indexOf("Wait ACK") >= 0) {
            waitingForAck = true;
            ackStartTime = millis();
        }
        
        // Look for ACK received
        if (response.indexOf("ACK Received") >= 0) {
            if (ackStartTime > 0) {
                ackTime = millis() - ackStartTime;
            }
            return true;
        }
        
        // Check for transmission failure
        if (response.indexOf("TX Failed") >= 0 || response.indexOf("No ACK") >= 0) {
            return false;
        }
        
        delay(100);
    }
    
    return false; // Timeout
}

bool LoRaTransmitter::enterLowPowerMode()
{
    Serial.println("Entering LoRa transmitter low power mode...");
    return sendATCommand("AT+LOWPOWER", "OK", 3000);
}

bool LoRaTransmitter::wakeUp()
{
    Serial.println("Waking up LoRa transmitter...");
    // Send any character to wake up
    if (loraSerial) {
        loraSerial->println("AT");
        delay(100);
    }
    return sendATCommand("AT", "OK", 3000);
}

bool LoRaTransmitter::setAutoLowPowerMode(bool enable)
{
    String command = "AT+LOWPOWER=AUTOMODE,";
    command += enable ? "ON" : "OFF";
    Serial.printf("Setting transmitter auto low power mode: %s\n", enable ? "ON" : "OFF");
    return sendATCommand(command, "OK", 3000);
}

String LoRaTransmitter::getStatistics()
{
    String stats = "LoRa Transmitter Statistics:\n";
    stats += "Successful transmissions: " + String(successfulTransmissions) + "\n";
    stats += "Failed transmissions: " + String(failedTransmissions) + "\n";
    stats += "Total retries: " + String(totalRetries) + "\n";
    
    if (successfulTransmissions + failedTransmissions > 0) {
        float successRate = (float)successfulTransmissions / (successfulTransmissions + failedTransmissions) * 100.0;
        stats += "Success rate: " + String(successRate, 1) + "%\n";
    }
    
    if (lastTransmissionTime > 0) {
        stats += "Last transmission: " + String((millis() - lastTransmissionTime) / 1000) + " seconds ago\n";
    }
    
    if (lastAckTime > 0) {
        stats += "Last ACK: " + String((millis() - lastAckTime) / 1000) + " seconds ago\n";
    }
    
    return stats;
}

String LoRaTransmitter::getLastError()
{
    return lastError;
}

void LoRaTransmitter::clearStatistics()
{
    successfulTransmissions = 0;
    failedTransmissions = 0;
    totalRetries = 0;
    lastTransmissionTime = 0;
    lastAckTime = 0;
    lastError = "";
}

bool LoRaTransmitter::setConfiguration(const LoRaWANConfig &loraConfig)
{
    config = loraConfig;
    
    if (isInitialized) {
        // Reconfigure if already initialized
        return configureLoRaWAN();
    }
    
    return true; // Configuration will be applied during setup
}

LoRaWANConfig LoRaTransmitter::getConfiguration()
{
    return config;
}

bool LoRaTransmitter::isJoined()
{
    // Check join status by attempting a simple query
    return sendATCommand("AT+DADDR", "", 3000);
}

bool LoRaTransmitter::rejoin()
{
    Serial.println("Force rejoin to LoRaWAN network...");
    return joinNetwork();
}

bool LoRaTransmitter::sendRawHex(const String &hexData, uint8_t port, bool confirmed)
{
    String command = "AT+CMSGHEX=\"" + hexData + "\"";
    return sendATCommand(command, "Done", LORAWAN_TX_TIMEOUT);
}

String LoRaTransmitter::getDeviceInfo()
{
    String info = "";
    
    // Get device ID information
    clearSerialBuffer();
    if (sendATCommand("AT+ID", "", 3000)) {
        String idResponse = readResponse(2000);
        info += "Device ID: " + idResponse + "\n";
    }
    
    // Get version information
    clearSerialBuffer();
    if (sendATCommand("AT+VER", "", 3000)) {
        String verResponse = readResponse(2000);
        info += "Firmware: " + verResponse + "\n";
    }
    
    // Get current configuration
    info += "Mode: " + (currentMode == LoRaCommunicationMode::P2P ? String("P2P") : String("LoRaWAN")) + "\n";
    
    if (currentMode == LoRaCommunicationMode::LoRaWAN) {
        info += "Region: " + config.region + "\n";
        info += "Data Rate: " + String(config.dataRate) + "\n";
        info += "TX Power: " + String(config.transmitPower) + " dBm\n";
        info += "Join Mode: " + (config.otaa ? String("OTAA") : String("ABP")) + "\n";
    } else {
        info += "Frequency: " + String(P2P_FREQUENCY) + " Hz\n";
        info += "Spreading Factor: " + String(P2P_SPREADING_FACTOR) + "\n";
        info += "Bandwidth: " + String(P2P_BANDWIDTH) + "\n";
        info += "Power: " + String(P2P_POWER) + " dBm\n";
    }
    
    return info;
}

LoRaCommunicationMode LoRaTransmitter::getCurrentMode()
{
    return currentMode;
}

bool LoRaTransmitter::switchMode(LoRaCommunicationMode mode)
{
    if (!isInitialized) {
        lastError = "Transmitter not initialized";
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
        lastError = "Failed to switch to " + 
                   (mode == LoRaCommunicationMode::P2P ? String("P2P") : String("LoRaWAN")) + " mode";
        Serial.println(lastError);
    }
    
    return success;
}

String LoRaTransmitter::sendCommandWithFallback(const String &command, int maxRetries)
{
    if (!isInitialized) {
        lastError = "Transmitter not initialized";
        Serial.println(lastError);
        return "";
    }
    
    Serial.printf("Sending command with fallback: %s\n", command.c_str());
    
    // Try current mode first
    String response = sendCommand(command, LORAWAN_PORT_CONTROL, true, maxRetries);
    if (response.length() > 0) {
        return response;
    }
    
    // If current mode failed, try the other mode
    LoRaCommunicationMode fallbackMode = (currentMode == LoRaCommunicationMode::P2P) ? 
                                         LoRaCommunicationMode::LoRaWAN : 
                                         LoRaCommunicationMode::P2P;
    
    Serial.printf("Primary mode failed, trying fallback mode: %s\n", 
                  fallbackMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
    
    if (switchMode(fallbackMode)) {
        response = sendCommand(command, LORAWAN_PORT_CONTROL, true, maxRetries);
        if (response.length() > 0) {
            Serial.printf("Fallback successful with %s mode\n", 
                         fallbackMode == LoRaCommunicationMode::P2P ? "P2P" : "LoRaWAN");
            return response;
        }
    }
    
    lastError = "Both P2P and LoRaWAN modes failed";
    Serial.println(lastError);
    return "";
}